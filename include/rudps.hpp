
#ifndef RUDPS_HPP
#define RUDPS_HPP

#include <cmac.hpp>
#include <frame_timer.hpp>
#include <send_queue.hpp>
#include <vector>
#include <deque>
#include <list>

#define SEND_INTERVAL 1e9
#define RUDPS_ELEMENT_SIZE 6

struct ShortMessage
{
    uint8_t data[UINT8_MAX];
    uint8_t size;
};

struct HeaderRUDPS
{
    uint8_t mac[CMAC_MAC_SIZE];
    uint32_t counter;
    uint16_t id;
    uint16_t seq;
    uint16_t ack;
};

constexpr int HeaderSize() noexcept
{
    constexpr HeaderRUDPS header{};
    constexpr int size =
        sizeof(header.mac) +
        sizeof(header.counter) +
        sizeof(header.id) +
        sizeof(header.seq) +
        sizeof(header.ack);
    return size;
}

class RUDPS
{
private:
    sockaddr_in remote_address;
    uint8_t* key;
    HeaderRUDPS header;
    std::list<std::unique_ptr<iovec>> recv_messages;
    std::deque<std::unique_ptr<ShortMessage>> send_messages;
    FrameTimer timer_for_send;
public:

    void Setup(long nano, uint8_t* key_) noexcept
    {
        key = key_;
        timer_for_send.Setup(nano);
    }

    void UpdateRecv(
            CMAC& cmac, 
            std::unique_ptr<HeaderRUDPS> header,
            std::unique_ptr<uint8_t> data,
            int len) noexcept
    {
        cmac.Init(key);
        cmac.Update(header->counter);
        cmac.Update(header->id);
        cmac.Update(header->seq);
        cmac.Update(header->ack);

        constexpr int header_size = HeaderSize();
        if (len == header_size)
        {
            cmac.Final(this->header.mac);
            if (memcmp(this->header.mac, header->mac, CMAC_MAC_SIZE) != 0)
            {
                return;
            }
        }
        else if (len > header_size)
        {
            std::list<std::unique_ptr<iovec>> iovecs;
            for (int offset = 0; offset < len; )
            {
                uint8_t seq_len = *(uint8_t*)(data.get() + offset);
                auto f = [&] 
                {
                    uint8_t* seq_data = data.get() + offset + sizeof(uint8_t);
                    auto iovec_ptr = std::make_unique<iovec>();
                    iovec_ptr->iov_base = seq_data;
                    iovec_ptr->iov_len = seq_len;
                    iovecs.push_back(std::move(iovec_ptr));
                    cmac.Update(seq_data, seq_len);
                    offset += seq_len;
                };
                size_t seq_size = header_size + offset + sizeof(uint8_t) + seq_len;
                if ((size_t)len == seq_size)
                {
                    f();
                    break;
                }
                else if ((size_t)len > seq_size)
                {
                    f();
                }
                else
                {
                    return;
                }
            }

            cmac.Final(this->header.mac);
            if (memcmp(this->header.mac, header->mac, CMAC_MAC_SIZE) != 0)
            {
                return;
            }

            for (; this->header.ack != (header->seq + 1); ++this->header.ack)
            {
                recv_messages.push_back(std::move(iovecs.back()));
                iovecs.pop_back();
            }

            for (; this->header.seq != (header->ack + 1); ++this->header.seq)
            {
                send_messages.pop_front();
            }
        }
    }

    std::unique_ptr<iovec> Pop() noexcept
    {
        auto ptr = std::move(recv_messages.front());
        recv_messages.pop_front();
        return std::move(ptr);
    }

    void Push(std::unique_ptr<ShortMessage> message) noexcept
    {
        send_messages.push_back(std::move(message));
    }

    void UpdateSend(Frame& frame, CMAC& cmac, SendQueue& send_queue) noexcept
    {
        timer_for_send.Update(frame);

        if (timer_for_send.IsExpired())
        {
            auto packet = std::make_unique<Packet>();
            auto& iovecs = packet->iovecs;
            iovecs.resize(RUDPS_ELEMENT_SIZE);
            size_t i = 0;
            iovecs[i].iov_base  = header.mac;
            iovecs[i++].iov_len = sizeof(header.mac);
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
                iovecs[i++].iov_len = sizeof(message->data);
                cmac.Update(message->data, message->size);
            }
            cmac.Final(header.mac);
            send_queue.Push(std::move(packet));
            timer_for_send.Reset();
        }
    }
};


int RUDPS_TEST()
{
    static uint8_t key[CMAC_KEY_SIZE] = {0};

    RUDPS rudps;
    rudps.Setup(SEND_INTERVAL, key);

    Frame frame;
    CMAC cmac;
    SendQueue send_queue;

    int is_server;

    printf("0 or 1 : ");
    scanf("%d", &is_server);
    printf("\n");

    int fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (is_server)
    {
        sockaddr_in addr = {0};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(4888);
        addr.sin_addr.s_addr = INADDR_ANY;
        bind(fd, (sockaddr*)&addr, sizeof(sockaddr_in));
    }
    else
    {
        sockaddr_in addr = {0};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(4888);
        addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    }
    
    msghdr recv_mh;
    iovec iovecs[6];
    recv_
    auto header = std::make_unique<HeaderRUDPS>();
    size_t i = 0;
    iovecs[i].iov_base  = header->mac;
    iovecs[i++].iov_len = sizeof(header->mac);
    iovecs[i].iov_base  = &header->counter;
    iovecs[i++].iov_len = sizeof(header->counter);
    iovecs[i].iov_base  = &header->id;
    iovecs[i++].iov_len = sizeof(header->id);
    iovecs[i].iov_base  = &header->seq;
    iovecs[i++].iov_len = sizeof(header->seq);
    iovecs[i].iov_base  = &header->ack;
    iovecs[i++].iov_len = sizeof(header->ack);
    while (true)
    {
        frame.Update();

        rudps.UpdateRecv(cmac, 
        rudps.UpdateSend(frame, cmac, send_queue);
    }

    return 0;
}

#endif

