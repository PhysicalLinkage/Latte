
#ifndef RUDPS_SERVER_HPP
#define RUDPS_SERVER_HPP

#include <unordered_map>

#include <openssl/rand.h>

#include <server_udp.hpp>
#include <rudps.hpp>

class ServerRUDPS;

struct SRUDPS : public RUDPS
{
public:
    bool is_used;
    uint16_t login_id;
    std::unique_ptr<sockaddr_in> address;
    explicit SRUDPS(ServerRUDPS& server_, std::unique_ptr<sockaddr_in> address_) noexcept
        : is_used {false}
        , address {std::move(address_)}
        , server {server_}
        , timer {}
    {
        timer.Setup(5e9);
    }

    
    bool Update(Frame& frame) noexcept
    {
        timer.Update(frame);
        if (timer.IsExpired())
        {
            timer.Reset();
            is_used = false;
        }

        return is_used;
    }
     
    void Update(sockaddr_in& address_) noexcept
    {
        *address = address_;
        timer.Reset();
    }

private:
    void OnRecv(
            std::unique_ptr<std::deque<iovec>>&& iovs,
            std::unique_ptr<Message>&& message) noexcept override;

    void OnSend(std::unique_ptr<std::vector<iovec>>&& iovs) noexcept override;

    ServerRUDPS& server;
    FrameTimer timer;
};

class ServerRUDPS : public Server::UDP
{
    friend class SRUDPS;
public:
    explicit ServerRUDPS(uint16_t port) noexcept
        : Server::UDP {port}
        , on_recvs {}
        , contactss {}
        , rudpss {}
        , dhl {}
        , cmac {}
    {
        on_recvs[TYPE_RUDPS_CONTACT] = &ServerRUDPS::OnRecvContact;
        on_recvs[TYPE_RUDPS_AUTH]    = &ServerRUDPS::OnRecvAuth;
        on_recvs[TYPE_RUDPS_MESSAGE] = &ServerRUDPS::OnRecvMessage;
        on_recvs[TYPE_RUDPS_LOGOUT]  = &ServerRUDPS::OnRecvEmpty;
    }

    

protected:
    virtual void OnRecv(
        SRUDPS& rudps,
        iovec& iov,
        std::shared_ptr<Message> message) = 0;
private:
    void OnRecv(
        std::unique_ptr<sockaddr_in>&& address, 
        std::unique_ptr<Message>&& message) noexcept override
    {
        constexpr uint8_t type_rudps_max = TYPE_RUDPS_SIZE - 1;
        uint8_t type = message->data[0] & type_rudps_max;
        (this->*on_recvs[type])(std::move(address), std::move(message));
    }

    struct Contact
    {
        uint8_t type;
        uint8_t nonce[RUDPS_CONTACT_NONCE_BYTES];
        bool auth;
        std::unique_ptr<sockaddr_in> address;
    };

    void OnRecvContact(
        std::unique_ptr<sockaddr_in>&& address, 
        std::unique_ptr<Message>&& message) noexcept
    {
        if (message->size == RUDPS_CONTACT_REQUEST_BYTES)
        {
            auto contacts_pair = contactss.find(address->sin_addr.s_addr);
            if ((contacts_pair != contactss.end()) &&
                (contacts_pair->second.count(address->sin_port) != 0))
            {
                if (contacts_pair->second[address->sin_port])
                {
                    printf("no contact: %u, %u\n", address->sin_addr.s_addr, address->sin_port);
                    return;
                }
            }

            printf("contact: %u, %u\n", address->sin_addr.s_addr, address->sin_port);

            auto& contact = contactss[address->sin_addr.s_addr][address->sin_port];
            contact = std::make_unique<Contact>();
            contact->type = message->data[0];
            if (RAND_bytes(contact->nonce, sizeof(RUDPS_CONTACT_NONCE_BYTES)))
            {
                auto iovs_ptr = std::make_unique<std::vector<iovec>>(3);
                auto& iovs = *iovs_ptr;
                iovs[0].iov_base    = &contact->type;
                iovs[0].iov_len     = sizeof(contact->type);
                iovs[1].iov_base    = contact->nonce;
                iovs[1].iov_len     = sizeof(contact->nonce);
                iovs[2].iov_base    = dhl.public_key;
                iovs[2].iov_len     = sizeof(dhl.public_key);
                contact->address = std::move(address);
                contact->auth = false;
                Send(std::make_unique<Server::SendPacket>(
                            *contact->address, std::move(iovs_ptr)));
            }
            else
            {
                printf("rand_bytes error\n");
            }
        }
    }

    void OnRecvAuth(
        std::unique_ptr<sockaddr_in>&& address, 
        std::unique_ptr<Message>&& message) noexcept
    {
        if (message->size != RUDPS_AUTH_REQUEST_BYTES)
        {
            printf("%u\n", message->size);
            printf("1\n");
            return;
        }
           
        auto contacts_pair = contactss.find(address->sin_addr.s_addr);    
        if (contacts_pair == contactss.end())
        {
            printf("2\n");
            return;
        }
                
        auto contact_pair = contacts_pair->second.find(address->sin_port);
        if (contact_pair == contacts_pair->second.end())
        {
            printf("3\n");
            return;
        }
        
        auto& contact = *contact_pair->second;

        if (contact.auth)
        {
            printf("contact.auth error\n");
            return;
        }

        if (memcmp(contact.nonce, message->data + RUDPS_TYPE_BYTES, sizeof(contact.nonce)) != 0)
        {
            printf("4\n");
            return;
        }

        contact.auth = true;

        printf("auth\n");
        for (size_t i = 0; i < rudpss.size(); ++i)
        {
            if (!rudpss[i]->is_used)
            {
                rudpss[i] = std::make_unique<SRUDPS>(*this, std::move(address));
                constexpr uint8_t key_offset = RUDPS_TYPE_BYTES + RUDPS_CONTACT_NONCE_BYTES;
                rudpss[i]->InitKey(dhl, message->data + key_offset);
                rudpss[i]->InitID(i);
                rudpss[i]->InitTimer(1e9);
                rudpss[i]->is_used = true;
                return;
            }
        }
        rudpss.push_back(std::make_unique<SRUDPS>(*this, std::move(address)));
        constexpr uint8_t key_offset = RUDPS_TYPE_BYTES + RUDPS_CONTACT_NONCE_BYTES;
        rudpss.back()->InitKey(dhl, message->data + key_offset);
        rudpss.back()->InitID(rudpss.size() - 1);
        rudpss.back()->InitTimer(1e9);
        rudpss.back()->is_used = true;
    }


    void OnRecvMessage(
        std::unique_ptr<sockaddr_in>&& address, 
        std::unique_ptr<Message>&& message) noexcept
    {
        constexpr size_t header_size = HEADER_RUDPS_SIZE;
        size_t data_size = message->size - 1;
        uint8_t* data = message->data + 1;
        if (data_size < header_size)
        {
            printf("1");
            return;
        }
        HeaderRUDPS header;
        header.Init(data);
        if (header.id >= rudpss.size())
        {
            printf("2");
            return;
        }

        auto& rudps = rudpss[header.id];

        auto f = [&]
        {
            uint32_t host = rudps->address->sin_addr.s_addr;
            uint16_t port = rudps->address->sin_port;
            uint32_t next_host = address->sin_addr.s_addr;
            uint16_t next_port = address->sin_port;
            if (host != next_host || port != next_port)
            {
                contactss[next_host][next_port] = std::move(contactss[host][port]);
                contactss[host].erase(port);
            }
            rudps->Update(*address);
        };
        rudps->RecvUpdate(cmac, std::move(message), f);
    }


    void OnRecvEmpty(
        std::unique_ptr<sockaddr_in>&& address, 
        std::unique_ptr<Message>&& message) noexcept
    {

    }

    void (ServerRUDPS::*on_recvs[TYPE_RUDPS_SIZE])(
        std::unique_ptr<sockaddr_in>&& address, 
        std::unique_ptr<Message>&& message);
protected:
    std::unordered_map<uint32_t, std::unordered_map<uint16_t, std::unique_ptr<Contact>>> contactss;
    std::vector<std::unique_ptr<SRUDPS>> rudpss;
protected:
    DHL dhl;
    CMAC cmac;
};

void SRUDPS::OnRecv(
        std::unique_ptr<std::deque<iovec>>&& iovs,
        std::unique_ptr<Message>&& message) noexcept
{
    if (is_used)
    {
        std::shared_ptr msg(std::move(message));
        size_t max_of_i = iovs->size() - 1;
        for (long i = max_of_i; i >= 0; --i)
        {
            server.OnRecv(*this, (*iovs)[i], msg);
        }
    }
}

void SRUDPS::OnSend(std::unique_ptr<std::vector<iovec>>&& iovs) noexcept
{
    if (is_used)
    {
        server.Send(std::make_unique<Server::SendPacket>(*address, std::move(iovs)));
    }
}

#endif

