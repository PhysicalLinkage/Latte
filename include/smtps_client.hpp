
#ifndef SMTPS_CLIENT_HPP
#define SMTPS_CLIENT_HPP

#include <udp_client.hpp>
#include <type_smtps.hpp>

class SMTPSClient : public UDPClient
{
public:
    explicit SMTPSClient(uint16_t port, const char* address) noexcept
        : UDPClient {port, address}
        , header {}
        , recv_ack {0}
    {
        connected = false;
        OnRecvs[TYPE_SMTPS_CONTACT] = &SMTPSClient::OnRecvContact;
        OnRecvs[TYPE_SMTPS_AUTH] = &SMTPSClient::OnRecvEmpty;
        OnRecvs[TYPE_SMTPS_MESSAGE] = &SMTPSClient::OnRecvAuth; 
        OnRecvs[TYPE_SMTPS_LOGOUT] = &SMTPSClient::OnRecvEmpty;
    }

    void Contact() noexcept
    {
        type = TYPE_SMTPS_CONTACT;
        auto iovs = std::make_unique<std::vector<iovec>>(1);
        iovs->front().iov_base = &type;
        iovs->front().iov_len = sizeof(type);
        UDPClient::Send(std::move(iovs));
    }

    void Update() noexcept
    {
        RecvUpdate();
        UpdateSend();
        SendUpdate();
    }
private:
    void OnRecv(std::unique_ptr<Packet>&& packet_) noexcept override
    {
        constexpr uint8_t type_smtps_max = TYPE_SMTPS_SIZE - 1;
        uint8_t type = packet_->data[0] & type_smtps_max;
        printf("type = %s\n", type_smtps_to_string(type));
        (this->*OnRecvs[type])(std::move(packet_));
    }

    class ContactDisposable : public IDisposable
    {
    public:
        explicit ContactDisposable(std::unique_ptr<Packet>&& packet_) noexcept
            : packet {std::move(packet_)} {}
        void Dispose() noexcept override {}
    private:
        std::unique_ptr<Packet> packet;
    };

    void OnRecvContact(std::unique_ptr<Packet>&& packet_) noexcept
    {
        if (packet_->size != SMTPS_CONTACT_REPLY_BYTES)
        {
            return;
        }

        size_t public_key_offset = SMTPS_TYPE_BYTES + SMTPS_CONTACT_NONCE_BYTES;
        dhl.ComputeKey(key, packet_->data + public_key_offset);

        printf("key = ");
        for (size_t i = 0; i < CMAC_KEY_SIZE; ++i)
        {
            printf("%x", key[i]);
        }
        printf("\n");

        auto iovs_ = std::make_unique<std::vector<iovec>>(2);
        auto& iovs = *iovs_;
        packet_->data[0] = TYPE_SMTPS_AUTH;
        iovs[0].iov_base = packet_->data;
        iovs[0].iov_len = SMTPS_TYPE_BYTES + SMTPS_CONTACT_NONCE_BYTES;
        iovs[1].iov_base = dhl.public_key;
        iovs[1].iov_len = sizeof(dhl.public_key);
        UDPClient::Send(std::move(iovs_), std::make_unique<ContactDisposable>(std::move(packet_)));
        printf("contact complete\n");
    }

    void OnRecvAuth(std::unique_ptr<Packet>&& packet_) noexcept
    {
        if (packet_->size != SMTPS_AUTH_REPLY_BYTES)
        {
            return;
        }
        
        size_t data_size = packet_->size - SMTPS_TYPE_BYTES;
        uint8_t* data = packet_->data + SMTPS_TYPE_BYTES;

        HeaderSMTPS header;
        header.Setup(data);

        cmac.Init(key);
        cmac.Update(header.counter);
        cmac.Update(header.id);
        cmac.Update(header.seq);
        cmac.Update(header.ack);

        constexpr int header_size = HeaderSMTPSSize();
        if (data_size == header_size)
        {
            cmac.Final(mac);
            if (memcmp(mac, header.mac, CMAC_MAC_SIZE) != 0)
            {
                printf("2\n");
                return;
            }

            Start();
            OnRecvs[TYPE_SMTPS_MESSAGE] = &SMTPSClient::OnRecvMessage;
        }
    }

    void OnRecvMessage(std::unique_ptr<Packet>&& packet_) noexcept
    {
        size_t data_size = packet_->size - SMTPS_TYPE_BYTES;
        uint8_t* data = packet_->data + SMTPS_TYPE_BYTES;

        constexpr int header_size = HeaderSMTPSSize();
        if (data_size < header_size)
        {
            return;
        }

        HeaderSMTPS header;
        header.Setup(data);

        cmac.Init(key);
        cmac.Update(header.counter);
        cmac.Update(header.id);
        cmac.Update(header.seq);
        cmac.Update(header.ack);

        printf("seq = %u\n", this->header.seq);
        printf("ack = %u\n", this->header.ack);
        printf("recv seq = %u\n", header.seq);
        printf("recv ack = %u\n", header.ack);
        printf("recv data= %u\n", data_size);
        printf("-------\n");
        printf("\n");

        if (data_size == header_size)
        {
            cmac.Final(mac);
            if (memcmp(mac, header.mac, CMAC_MAC_SIZE) != 0)
            {
                return;
            }

            for (; recv_ack != header.ack; ++recv_ack)
            {
                send_messages.pop_front();
            }
        }
        else if (data_size > header_size)
        {
            std::list<std::unique_ptr<iovec>> iovecs;
            for (int offset = header_size; offset < data_size; )
            {
                uint8_t seq_len = *(uint8_t*)(data + offset);
                size_t seq_size = offset + sizeof(uint8_t) + seq_len;
                if ((size_t)data_size == seq_size)
                {
                    uint8_t* seq_data = data + offset + sizeof(uint8_t);
                    auto iovec_ptr = std::make_unique<iovec>();
                    iovec_ptr->iov_base = seq_data;
                    iovec_ptr->iov_len = seq_len;
                    iovecs.push_back(std::move(iovec_ptr));
                    cmac.Update(seq_data, seq_len);
                    offset += seq_len + sizeof(uint8_t);
                    break;
                }
                else if ((size_t)data_size > seq_size)
                {
                    uint8_t* seq_data = data + offset + sizeof(uint8_t);
                    auto iovec_ptr = std::make_unique<iovec>();
                    iovec_ptr->iov_base = seq_data;
                    iovec_ptr->iov_len = seq_len;
                    iovecs.push_back(std::move(iovec_ptr));
                    cmac.Update(seq_data, seq_len);
                    offset += seq_len + sizeof(uint8_t);
                }
                else
                {
                    return;
                }
            }


            cmac.Final(mac);
            if (memcmp(mac, header.mac, CMAC_MAC_SIZE) != 0)
            {
                return;
            }

            for (; this->header.ack != header.seq; ++this->header.ack)
            {
                OnRecv(std::move(iovecs.back()));
                iovecs.pop_back();
            }

            printf("recv_ack = %u\n", recv_ack);
            printf("header.ack %u\n", header.ack);

            for (; recv_ack != header.ack; ++recv_ack)
            {
                send_messages.pop_front();
            }
        }
    }
    

    void OnRecvLogout(std::unique_ptr<Packet>&& packet_) noexcept
    {

    }

    void OnRecvEmpty(std::unique_ptr<Packet>&&) noexcept
    {
    }

    void Start() noexcept
    {
        auto sm = std::make_unique<ShortMessage>();
        uint8_t s[] = "Hello smtps!!\n";
        memcpy(sm->data, s, sizeof(s));
        sm->size = sizeof(s);
        SendC(std::move(sm));
        connected = true;
        timer_for_send.Setup(1e9);
    }

    void OnRecv(std::unique_ptr<iovec>&& iov) noexcept
    {
        printf("client ok\n");
    }

    void SendC(std::unique_ptr<ShortMessage>&& message) noexcept
    {
        ++header.seq;
        send_messages.push_back(std::move(message));
    }

    void UpdateSend() noexcept
    {
        if (connected == false)
        {
            return;
        }

        timer_for_send.Update();

        if (timer_for_send.IsExpired())
        {
            auto iovecs_ptr = std::make_unique<std::vector<iovec>>(1 + RUDPS_ELEMENT_SIZE + send_messages.size());
            auto& iovecs = *iovecs_ptr;
            size_t i = 0;
            static uint8_t type = 2;
            iovecs[i].iov_base  = &type;
            iovecs[i++].iov_len = sizeof(type);
            iovecs[i].iov_base  = mac;
            iovecs[i++].iov_len = sizeof(mac);
            cmac.Init(key);
            iovecs[i].iov_base  = &header.counter;
            iovecs[i++].iov_len = sizeof(header.counter);
            cmac.Update(header.counter);
            iovecs[i].iov_base  = &header.id;
            iovecs[i++].iov_len = sizeof(header.id);
            cmac.Update(header.id);
            iovecs[i].iov_base  = &header.seq;
            iovecs[i++].iov_len = sizeof(header.seq);
            cmac.Update(header.seq);
            iovecs[i].iov_base  = &header.ack;
            iovecs[i++].iov_len = sizeof(header.ack);
            cmac.Update(header.ack);
            for (size_t j = 0; j < send_messages.size(); ++j)
            {
                auto& message = send_messages[j];
                iovecs[i].iov_base  = &message->size;
                iovecs[i++].iov_len = sizeof(message->size);
                iovecs[i].iov_base  = message->data;
                iovecs[i++].iov_len = message->size;
                cmac.Update(message->data, message->size);
            }
            cmac.Final(mac);
            UDPClient::Send(std::move(iovecs_ptr));
            timer_for_send.Reset();
        }
    }

    void (SMTPSClient::*OnRecvs[TYPE_SMTPS_SIZE])
        (std::unique_ptr<Packet>&& packets_);
    bool connected;
    uint8_t type;
    DHL dhl;
    uint8_t key[DHL_KEY_SIZE];
    uint8_t mac[CMAC_MAC_SIZE];
    HeaderSMTPS header;
    CMAC cmac;
    std::deque<std::unique_ptr<ShortMessage>> send_messages;
    FrameTimer timer_for_send;
    uint16_t recv_ack;
};

#endif

