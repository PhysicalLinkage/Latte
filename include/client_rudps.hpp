
#ifndef CLIENT_RUDPS_HPP
#define CLIENT_RUDPS_HPP

#include <rudps.hpp>


class ClientRUDPS : public Client::UDP, public RUDPS
{
public:
    explicit ClientRUDPS(uint16_t port, const char* address) noexcept
        : Client::UDP {port, address}
        , update {&ClientRUDPS::UdpUpdate}
        , on_recvs {}
        , on_recv_type {0}
        , dhl {}
        , cmac {}
        , nano {(long)1e9}
    {
        on_recvs[TYPE_RUDPS_CONTACT] = &ClientRUDPS::OnRecvContact;
        on_recvs[TYPE_RUDPS_AUTH]    = &ClientRUDPS::OnRecvEmpty;
        on_recvs[TYPE_RUDPS_MESSAGE] = &ClientRUDPS::OnRecvAuth; 
        on_recvs[TYPE_RUDPS_LOGOUT]  = &ClientRUDPS::OnRecvEmpty;
    }

    void InitTimer(long nano_) noexcept
    {
        nano = nano_;
    }

    void Contact() noexcept
    {
        on_recv_type = TYPE_RUDPS_CONTACT;
        auto iovs = std::make_unique<std::vector<iovec>>(1);
        iovs->front().iov_base = &on_recv_type;
        iovs->front().iov_len = sizeof(on_recv_type);
        auto sp = std::make_unique<Client::SendPacket>(std::move(iovs));
        Client::UDP::Send(std::move(sp));
    }

    void Update(Frame& frame) noexcept
    {
        (this->*update)(frame);       
    }

protected:
    // from UDP
    void OnRecv(std::unique_ptr<Message>&& message) noexcept override
    {
        constexpr uint8_t type_rudps_max = TYPE_RUDPS_SIZE - 1;
        on_recv_type = message->data[0] & type_rudps_max;
        (this->*on_recvs[on_recv_type])(std::move(message));
    }

    // virtual from RUDPS
    void OnRecv(
            std::unique_ptr<std::vector<iovec>>&& iovs,
            std::unique_ptr<Message>&& message) noexcept override
    {
        for (long i = iovs->size() - 1; i >= 0; --i)
        {
            printf("recv message : %s\n", (char*)(*iovs)[i].iov_base);
        }
        printf("-----------------\n\n");
    }

    // from RUDPS
    void OnSend(std::unique_ptr<std::vector<iovec>>&& iovs) noexcept override
    {
        Client::UDP::Send(std::make_unique<Client::SendPacket>(std::move(iovs)));
    }

    // virtual from this
    void Start() noexcept
    {
        auto sm = std::make_unique<RUDPS::SendPacket>();
        sm->iovs.resize(1);
        static uint8_t s[] = "Hello smtps!!";
        sm->iovs[0].iov_base = s;
        sm->iovs[0].iov_len = sizeof(s);
        RUDPS::Send(std::move(sm));
    }

private:
    void UdpUpdate(Frame&) noexcept
    {
        Client::UDP::RecvUpdate();
        Client::UDP::SendUpdate();
    }

    void RudpsUpdate(Frame& frame) noexcept
    {
        Client::UDP::RecvUpdate();
        RUDPS::SendUpdate(frame, cmac);
        Client::UDP::SendUpdate();
    }
private:
    struct ContactPacket : public Client::SendPacket
    {
        std::unique_ptr<Message> message;
    };

    void OnRecvContact(std::unique_ptr<Message>&& message) noexcept
    {
        if (message->size != RUDPS_CONTACT_REPLY_BYTES)
        {
            return;
        }

        size_t public_key_offset = RUDPS_TYPE_BYTES + RUDPS_CONTACT_NONCE_BYTES;
        InitKey(dhl, message->data + public_key_offset);
        InitSeqAck();

        auto iovs_ptr = std::make_unique<std::vector<iovec>>(2);
        auto& iovs = *iovs_ptr;
        message->data[0] = TYPE_RUDPS_AUTH;
        iovs[0].iov_base = message->data;
        iovs[0].iov_len = RUDPS_TYPE_BYTES + RUDPS_CONTACT_NONCE_BYTES;
        iovs[1].iov_base = dhl.public_key;
        iovs[1].iov_len = sizeof(dhl.public_key);
        auto packet = std::make_unique<ContactPacket>();
        packet->iovs = std::move(iovs_ptr);
        packet->message = std::move(message);
        Client::UDP::Send(std::move(packet));
        printf("contact complete\n");
    }

    void OnRecvAuth(std::unique_ptr<Message>&& message) noexcept
    {
        printf("Auth\n");
        uint8_t* data = message->data + RUDPS_TYPE_BYTES;
        uint16_t data_size = message->size - RUDPS_TYPE_BYTES;

        constexpr int header_size = HEADER_RUDPS_SIZE;
        if (data_size < header_size)
        {
            return;
        }

        HeaderRUDPS header;
        header.Init(data);
        
        cmac.Init(key);
        cmac.Update(header.counter);
        cmac.Update(header.id);
        cmac.Update(header.seq);
        cmac.Update(header.ack);

        if (data_size == header_size)
        {
            cmac.Final(mac);
            if (memcmp(mac, header.mac, CMAC_MAC_SIZE) != 0)
            {
                return;
            }


            for (; recv_ack != header.ack; ++recv_ack)
            {
                RUDPS::send_packets.pop_front();
            }

            InitID(header.id);
            RUDPS::InitTimer(nano);
            Start();
            update = &ClientRUDPS::RudpsUpdate;
            on_recvs[TYPE_RUDPS_MESSAGE] = &ClientRUDPS::OnRecvMessage;
        }
    }

    void OnRecvMessage(std::unique_ptr<Message>&& message) noexcept
    {
        RUDPS::RecvUpdate(cmac, std::move(message));
    }

    void OnRecvEmpty(std::unique_ptr<Message>&&) noexcept
    {
    }
protected:

    void (ClientRUDPS::*update)(Frame& frame);
    void (ClientRUDPS::*on_recvs[TYPE_RUDPS_SIZE])
        (std::unique_ptr<Message>&& message);
    uint8_t on_recv_type;
    DHL dhl;
    CMAC cmac;
    long nano;
};

#endif

