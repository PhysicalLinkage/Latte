
#ifndef SMTPS_HPP
#define SMTPS_HPP

#include <arpa/inet.h>

#include <memory>
#include <vector>
#include <deque>
#include <list>

#include <dhl.hpp>
#include <cmac.hpp>
#include <frame_timer.hpp>

#include <udp_server.hpp>

#define SEND_INTERVAL 1e9
#define RUDPS_ELEMENT_SIZE 6

struct ShortMessage
{
    uint8_t data[UINT8_MAX];
    uint8_t size;
};

struct HeaderSMTPS
{
    uint8_t* mac;
    uint32_t counter;
    uint16_t id;
    uint16_t seq;
    uint16_t ack;

    void Setup(uint8_t* data)
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


constexpr int HeaderSMTPSSize() noexcept
{
    constexpr HeaderSMTPS header{};
    constexpr int size =
        CMAC_MAC_SIZE +
        sizeof(header.counter) +
        sizeof(header.id) +
        sizeof(header.seq) +
        sizeof(header.ack);
    return size;
}

class SMTPS
{
private:
    uint8_t mac[CMAC_MAC_SIZE];
    HeaderSMTPS header;
    uint16_t recv_ack;
    std::deque<std::unique_ptr<ShortMessage>> send_messages;
    FrameTimer timer_for_send;
public:
    uint8_t key[DHL_KEY_SIZE];
    sockaddr_in address;

    void Setup(sockaddr_in& address_, long nano) noexcept
    {
        address = address_;
        timer_for_send.Setup(nano);
        recv_ack = header.seq = header.ack = 0;
    }

    void UpdateRecv(
            CMAC& cmac, 
            std::unique_ptr<UDPServer::Packet>&& packet) noexcept
    {
        uint8_t* data = packet->data + 1;
        uint16_t data_size = packet->size - 1;

        HeaderSMTPS header;

        header.Setup(data);

        cmac.Init(key);
        cmac.Update(header.counter);
        cmac.Update(header.id);
        cmac.Update(header.seq);
        cmac.Update(header.ack);

        printf("seq = %u\n", this->header.seq);
        printf("ack = %u\n", this->header.ack);
        printf("prev ack = %u\n", recv_ack);
        printf("recv ctr = %u\n", header.counter);
        printf("recv id  = %u\n", header.id);
        printf("recv seq = %u\n", header.seq);
        printf("recv ack = %u\n", header.ack);
        printf("recv data= %u\n", data_size);
        printf("-------\n");
        printf("\n");

        constexpr int header_size = HeaderSMTPSSize();
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
            auto iovs = std::make_unique<std::vector<iovec>>(16);
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
                    return;
                }
            }

            cmac.Final(mac);
            if (memcmp(mac, header.mac, CMAC_MAC_SIZE) != 0)
            {
                printf("mac error\n");
                return;
            }

            iovs->resize(header.seq - this->header.ack);
            OnRecv(std::move(iovs), std::move(packet));
            this->header.ack = header.seq;

            for (; recv_ack != header.ack; ++recv_ack)
            {
                send_messages.pop_front();
            }
        }
    }

    virtual void OnRecv(
            std::unique_ptr<std::vector<iovec>>&& iovs, 
            std::unique_ptr<UDPServer::Packet>&& packet) = 0;

    void Send(std::unique_ptr<ShortMessage>&& message) noexcept
    {
        ++header.seq;
        send_messages.push_back(std::move(message));
    }

    void UpdateSend(Frame& frame, CMAC& cmac, UDPServer::SendMethod& Send) noexcept
    {
        timer_for_send.Update(frame);

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
            auto addr = std::make_unique<sockaddr_in>();
            *addr = address;
            auto sp = std::make_unique<UDPServer::SendPacket>(
                    std::move(addr), std::move(iovecs_ptr));
            Send(std::move(sp));
            timer_for_send.Reset();
        
        }
    }
};

#endif

