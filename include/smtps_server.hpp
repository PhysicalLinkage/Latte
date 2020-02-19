
#ifndef SMTPS_SERVER_HPP
#define SMTPS_SERVER_HPP

#include <unordered_map>

#include <openssl/rand.h>

#include <udp_server.hpp>
#include <smtps.hpp>
#include <type_smtps.hpp>

template<class SMTPS_T>
class SMTPSServer : public UDPServer
{
public:
    explicit SMTPSServer(uint16_t port) noexcept
        : UDPServer {port}
        , OnRecvs {}
        , contactss {}
        , smtpss {}
        , dhl {}
        , cmac {}
    {
        OnRecvs[TYPE_SMTPS_CONTACT] = &SMTPSServer::OnRecvContact;
        OnRecvs[TYPE_SMTPS_AUTH]    = &SMTPSServer::OnRecvAuth;
        OnRecvs[TYPE_SMTPS_MESSAGE] = &SMTPSServer::OnRecvMessage;
        OnRecvs[TYPE_SMTPS_LOGOUT]  = &SMTPSServer::OnRecvLogout;
    }
    
    void Update() noexcept
    {
        RecvUpdate();
        for (auto& smtps : smtpss)
        {
            smtps->UpdateSend(cmac, Send);
        }
        SendUpdate();
    }

private:
    void OnRecv(
        std::unique_ptr<sockaddr_in>&& address_, 
        std::unique_ptr<Packet>&& packet_) noexcept override
    {
        constexpr uint8_t type_smtps_max = TYPE_SMTPS_SIZE - 1;
        uint8_t type = packet_->data[0] & type_smtps_max;
        printf("type = %s\n", type_smtps_to_string(type));
        (this->*OnRecvs[type])(std::move(address_), std::move(packet_));
    }

    struct Contact
    {
        uint8_t type;
        uint8_t nonce[SMTPS_CONTACT_NONCE_BYTES];
    };

    void OnRecvContact(
        std::unique_ptr<sockaddr_in>&& address_, 
        std::unique_ptr<Packet>&& packet_) noexcept
    {
        if (packet_->size == SMTPS_CONTACT_REQUEST_BYTES)
        {
            auto contacts_pair = contactss.find(address_->sin_addr.s_addr);
            if ((contacts_pair != contactss.end()) &&
                (contacts_pair->second.count(address_->sin_port) != 0))
            {
                return;
            }


            auto& contact = contactss[address_->sin_addr.s_addr][address_->sin_port];
            contact = std::make_unique<Contact>();
            contact->type = packet_->data[0];
            if (RAND_bytes(contact->nonce, sizeof(SMTPS_CONTACT_NONCE_BYTES)))
            {
                auto iovs_ptr = std::make_unique<std::vector<iovec>>(3);
                auto& iovs = *iovs_ptr;
                iovs[0].iov_base    = &contact->type;
                iovs[0].iov_len     = sizeof(contact->type);
                iovs[1].iov_base    = contact->nonce;
                iovs[1].iov_len     = sizeof(contact->nonce);
                iovs[2].iov_base    = dhl.public_key;
                iovs[2].iov_len    = sizeof(dhl.public_key);
                auto send_packet = std::make_unique<SendPacket>(
                    std::move(address_), std::move(iovs_ptr));
                Send(std::move(send_packet));
            }
        }
    }

    void OnRecvAuth(
        std::unique_ptr<sockaddr_in>&& address_, 
        std::unique_ptr<Packet>&& packet_) noexcept
    {
        if (packet_->size != SMTPS_AUTH_REQUEST_BYTES)
        {
            printf("%u\n", packet_->size);
            printf("1\n");
            return;
        }
           
        auto contacts_pair = contactss.find(address_->sin_addr.s_addr);    
        if (contacts_pair == contactss.end())
        {
            printf("2\n");
            return;
        }
                
        auto contact_pair = contacts_pair->second.find(address_->sin_port);
        if (contact_pair == contacts_pair->second.end())
        {
            printf("3\n");
            return;
        }
        
        auto& contact = *contact_pair->second;
        if (memcmp(contact.nonce, packet_->data + SMTPS_TYPE_BYTES, sizeof(contact.nonce)) != 0)
        {
            printf("4\n");
            return;
        }

        smtpss.push_back(std::make_unique<SMTPS_T>());
        constexpr uint8_t key_offset = SMTPS_TYPE_BYTES + SMTPS_CONTACT_NONCE_BYTES;
        dhl.ComputeKey(smtpss.back()->key, packet_->data + key_offset);
        
        printf("key = ");
        for (size_t i = 0; i < CMAC_KEY_SIZE; ++i)
        {
            printf("%x", smtpss.back()->key[i]);
        }
        printf("\n");

        long nano = 4e9;
        smtpss.back()->Setup(*address_, nano);
    }


    void OnRecvMessage(
        std::unique_ptr<sockaddr_in>&& address_, 
        std::unique_ptr<Packet>&& packet_) noexcept
    {
        constexpr size_t header_size = HeaderSMTPSSize();
        size_t data_size = packet_->size - 1;
        uint8_t* data = packet_->data + 1;
        if (data_size < header_size)
        {
            return;
        }
        HeaderSMTPS header;
        header.Setup(data);
        if (header.id >= smtpss.size())
        {
            return;
        }
        auto& smtps = smtpss[header.id];
        smtps->address = *address_;
        smtps->UpdateRecv(cmac, data, data_size);
    }


    void OnRecvLogout(
        std::unique_ptr<sockaddr_in>&& address_, 
        std::unique_ptr<Packet>&& packet_) noexcept
    {
    }

    void (SMTPSServer::*OnRecvs[TYPE_SMTPS_SIZE])(
        std::unique_ptr<sockaddr_in>&& address_, 
        std::unique_ptr<Packet>&& packet_);
    std::unordered_map<uint32_t, std::unordered_map<uint16_t, std::unique_ptr<Contact>>> contactss;
    std::vector<std::unique_ptr<SMTPS_T>> smtpss;
    DHL dhl;
    CMAC cmac;
};

#endif

