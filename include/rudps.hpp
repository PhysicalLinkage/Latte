
#ifndef RUDPS_HPP
#define RUDPS_HPP

#include <arpa/inet.h>

#include <memory>
#include <vector>
#include <deque>
#include <list>

#include <dhl.hpp>
#include <cmac.hpp>
#include <frame_timer.hpp>

#include <client_udp.hpp>

static constexpr double SEND_INTERVAL = 1e9;
static constexpr size_t RUDPS_ELEMENT_SIZE = 6;

static constexpr size_t TYPE_RUDPS_CONTACT = 0;
static constexpr size_t TYPE_RUDPS_AUTH    = 1;
static constexpr size_t TYPE_RUDPS_MESSAGE = 2;
static constexpr size_t TYPE_RUDPS_LOGOUT  = 3;
static constexpr size_t TYPE_RUDPS_SIZE    = 4;

const char* type_rudps_to_string(uint8_t type) noexcept
{
    constexpr static const char* strings[TYPE_RUDPS_SIZE] =
    {
        "TYPE_SMTPS_CONTACT",
        "TYPE_SMTPS_AUTH",
        "TYPE_SMTPS_MESSAGE",
        "TYPE_SMTPS_LOGOUT"
    };

    constexpr static const char* error = "Type is unknown.\n";

    return (type < TYPE_RUDPS_SIZE) ? strings[type] : error;
}

static constexpr int RUDPS_CONTACT_NONCE_BYTES = 12;

struct HeaderRUDPS
{
    uint8_t* mac;
    uint32_t counter;
    uint16_t id;
    uint16_t seq;
    uint16_t ack;

    void Init(uint8_t* data) noexcept
    {
        constexpr size_t mac_offset = 0;
        constexpr size_t counter_offset = mac_offset + CMAC_MAC_SIZE;
        constexpr size_t id_offset = counter_offset + sizeof(counter);
        constexpr size_t seq_offset = id_offset + sizeof(id);
        constexpr size_t ack_offset = seq_offset + sizeof(seq);

        mac     = data + mac_offset;
        counter = *(uint32_t*)(data + counter_offset);
        id      = *(uint16_t*)(data + id_offset);
        seq     = *(uint16_t*)(data + seq_offset);
        ack     = *(uint16_t*)(data + ack_offset);
    }
};

static constexpr size_t HEADER_RUDPS_SIZE = CMAC_MAC_SIZE
                                        + sizeof(uint32_t)
                                        + sizeof(uint16_t)
                                        + sizeof(uint16_t)
                                        + sizeof(uint16_t);
        
static constexpr size_t RUDPS_TYPE_BYTES = 1;
static constexpr size_t RUDPS_CONTACT_REQUEST_BYTES = RUDPS_TYPE_BYTES;
static constexpr size_t RUDPS_CONTACT_REPLY_BYTES = RUDPS_TYPE_BYTES
                                                + RUDPS_CONTACT_NONCE_BYTES
                                                + DHL_KEY_SIZE;
static constexpr size_t RUDPS_AUTH_REQUEST_BYTES = RUDPS_CONTACT_REPLY_BYTES;
static constexpr size_t RUDPS_AUTH_REPLY_BYTES = RUDPS_TYPE_BYTES + HEADER_RUDPS_SIZE;
static constexpr size_t RUDPS_LOGOUT_REQUEST_BYTES = RUDPS_TYPE_BYTES + HEADER_RUDPS_SIZE;

struct iovec8
{
    void* iov_base;
    uint8_t  iov_len;
};

#define IOV8_MAX 10

class RUDPS
{
public:
    CMAC m_cmac;

    struct SendPacket
    {
        std::vector<iovec8> iovs;
        uint8_t total;
    };

    explicit RUDPS() noexcept
    {
        recv_ack = header.seq = header.ack = 0;
    }

    void InitKey(DHL& dhl, uint8_t* public_key) noexcept
    {
        dhl.ComputeKey(key, public_key);
        printf("key = ");
        for (size_t i = 0; i < CMAC_KEY_SIZE; ++i)
        {
            printf("%x", key[i]);
        }
        printf("\n");
    }

    void InitID(uint16_t id) noexcept
    {
        header.id = id;
        printf("id = %u\n", id);
    }

    uint16_t& ID() noexcept
    {
        return header.id;
    }

    void InitTimer(long nano) noexcept
    {
        timer_for_send.Setup(nano);
    }

    template<class F>
    void  RecvUpdate(
            CMAC& cmac, 
            std::unique_ptr<Message>&& message,
            F f) noexcept
    {
        uint8_t* data = message->data + RUDPS_TYPE_BYTES;
        uint16_t data_size = message->size - RUDPS_TYPE_BYTES;
     
        constexpr int header_size = HEADER_RUDPS_SIZE;
        if (data_size < header_size)
        {
            printf("rudps 1\n");
            return;
        }

        HeaderRUDPS recv_header;
        recv_header.Init(data);

        cmac.Init(key);
        cmac.Update(recv_header.counter);
        cmac.Update(recv_header.id);
        cmac.Update(recv_header.seq);
        cmac.Update(recv_header.ack);



        if (data_size == header_size)
        {
            cmac.Final(mac);
            if (memcmp(mac, recv_header.mac, CMAC_MAC_SIZE) != 0)
            {
                printf("rudps 2\n");
                return;
            }
        }
        else if (data_size > header_size)
        {
            auto iovs = std::make_unique<std::deque<iovec>>(16);
            iovs->resize(0);
            for (int offset = header_size; offset < data_size; )
            {
                uint8_t seq_len = *(uint8_t*)(data + offset);
                size_t seq_size = offset + sizeof(uint8_t) + seq_len;
                if ((size_t)data_size == seq_size)
                {
                    uint8_t* seq_data = data + offset + sizeof(uint8_t);
                    iovs->resize(iovs->size() + 1);
                    iovs->back().iov_base = seq_data;
                    iovs->back().iov_len = seq_len;
                    cmac.Update(seq_data, seq_len);
                    offset += seq_len + sizeof(uint8_t);
                    break;
                }
                else if ((size_t)data_size > seq_size)
                {
                    uint8_t* seq_data = data + offset + sizeof(uint8_t);
                    iovs->resize(iovs->size() + 1);
                    iovs->back().iov_base = seq_data;
                    iovs->back().iov_len = seq_len;
                    cmac.Update(seq_data, seq_len);
                    offset += seq_len + sizeof(uint8_t);
                }
                else
                {
                    printf("rudps 3\n");
                    return;
                }
            }

            cmac.Final(mac);
            if (memcmp(mac, recv_header.mac, CMAC_MAC_SIZE) != 0)
            {
                printf("rudps 4\n");
                return;
            }

            size_t distance = recv_header.seq - this->header.ack;
            
                

            if (distance > UINT16_MAX / 2)
            {
                return;
            }        

            if (distance != 0)
            {
                /*
                printf("recv----------\n");
                Print();
                Print(recv_header, data_size);
                */
                while (iovs->size() != distance)
                {
                    iovs->pop_front();
                }
                OnRecv(std::move(iovs), std::move(message));
                this->header.ack = recv_header.seq;

            }

        }

        for (; recv_ack != recv_header.ack; ++recv_ack)
        {
            send_packets.pop_front();
        }

        f();
    }


    void Send(std::unique_ptr<SendPacket>&& send_packet) noexcept
    {
        ++header.seq;
        send_packets.push_back(std::move(send_packet));

        auto iovecs_ptr = std::make_unique<std::vector<iovec>>(
                1 + RUDPS_ELEMENT_SIZE + (send_packets.size() * IOV8_MAX));
        auto& iovecs = *iovecs_ptr;
        size_t i = 0;
        static uint8_t type = 2;
        iovecs[i].iov_base  = &type;
        iovecs[i++].iov_len = sizeof(type);
        iovecs[i].iov_base  = mac;
        iovecs[i++].iov_len = sizeof(mac);
        m_cmac.Init(key);
        ++header.counter;
        iovecs[i].iov_base  = &header.counter;
        iovecs[i++].iov_len = sizeof(header.counter);
        m_cmac.Update(header.counter);
        iovecs[i].iov_base  = &header.id;
        iovecs[i++].iov_len = sizeof(header.id);
        m_cmac.Update(header.id);
        iovecs[i].iov_base  = &header.seq;
        iovecs[i++].iov_len = sizeof(header.seq);
        m_cmac.Update(header.seq);
        iovecs[i].iov_base  = &header.ack;
        iovecs[i++].iov_len = sizeof(header.ack);
        m_cmac.Update(header.ack);
        
        for (auto& packet : send_packets)
        {
            size_t size_i = i++;
            packet->total = 0;
            for (auto iov : packet->iovs)
            {
                iovecs[i].iov_base  = iov.iov_base;
                iovecs[i++].iov_len = iov.iov_len;
                m_cmac.Update(iov.iov_base, iov.iov_len);
                packet->total += iov.iov_len;
            }
            iovecs[size_i].iov_base = &packet->total;
            iovecs[size_i].iov_len = sizeof(packet->total);
        }

        m_cmac.Final(mac);
        iovecs.resize(i);

        OnSend(std::move(iovecs_ptr));
        timer_for_send.Reset();
    }

    void SendUpdate(Frame& frame, CMAC& cmac) noexcept
    {
        timer_for_send.Update(frame);

        if (timer_for_send.IsExpired())
        {
            auto iovecs_ptr = std::make_unique<std::vector<iovec>>(
                    1 + RUDPS_ELEMENT_SIZE + (send_packets.size() * IOV8_MAX));
            auto& iovecs = *iovecs_ptr;
            size_t i = 0;
            static uint8_t type = 2;
            iovecs[i].iov_base  = &type;
            iovecs[i++].iov_len = sizeof(type);
            iovecs[i].iov_base  = mac;
            iovecs[i++].iov_len = sizeof(mac);
            cmac.Init(key);
            ++header.counter;
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
            
            for (auto& packet : send_packets)
            {
                size_t size_i = i++;
                packet->total = 0;
                for (auto iov : packet->iovs)
                {
                    iovecs[i].iov_base  = iov.iov_base;
                    iovecs[i++].iov_len = iov.iov_len;
                    cmac.Update(iov.iov_base, iov.iov_len);
                    packet->total += iov.iov_len;
                }
                iovecs[size_i].iov_base = &packet->total;
                iovecs[size_i].iov_len = sizeof(packet->total);
            }

            cmac.Final(mac);
            iovecs.resize(i);

            OnSend(std::move(iovecs_ptr));
            timer_for_send.Reset();
        }
    }
protected:
    virtual void OnRecv(
            std::unique_ptr<std::deque<iovec>>&& iovs,
            std::unique_ptr<Message>&& message) = 0;
    
    virtual void OnSend(
            std::unique_ptr<std::vector<iovec>>&& iovs) = 0;
    
    uint8_t mac[CMAC_MAC_SIZE];
private:
    HeaderRUDPS header;
protected:
    uint16_t recv_ack;
    std::deque<std::unique_ptr<SendPacket>> send_packets;
    FrameTimer timer_for_send;
    uint8_t key[DHL_KEY_SIZE];
private:

    void Print() noexcept
    {
        printf("key = ");
        for (size_t i = 0; i < CMAC_KEY_SIZE; ++i)
        {
            printf("%x", key[i]);
        }
        printf("\n");
        printf("mac = ");
        for (size_t i = 0; i < CMAC_MAC_SIZE; ++i)
        {
            printf("%x", mac[i]);
        }
        printf("\n");
        printf("counter = %u\n", header.counter);
        printf("id  = %u\n", header.id);
        printf("seq = %u\n", header.seq);
        printf("ack = %u\n", header.ack);
        printf("remote ack = %u\n", recv_ack);
    }

    void Print(HeaderRUDPS& header_, uint16_t data_size) noexcept
    {
        printf("recv mac = ");
        for (size_t i = 0; i < CMAC_MAC_SIZE; ++i)
        {
            printf("%x", header_.mac[i]);
        }
        printf("\n");
        printf("recv counter = %u\n", header_.counter);
        printf("recv id  = %u\n", header_.id);
        printf("recv seq = %u\n", header_.seq);
        printf("recv ack = %u\n", header_.ack);
        printf("recv data= %u\n", data_size);
    }
};

#endif

