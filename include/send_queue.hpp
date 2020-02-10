
#ifndef SEND_QUEUE
#define SEND_QUEUE

#include <arpa/inet.h>

#include <vector>
#include <deque>
#include <memory>

#include <packet.hpp>

#define MNSU 100

class SendQueue
{
private:
    mmsghdr mmh[MNSU];
    std::deque<std::unique_ptr<Packet>> packets;

    size_t Setup() noexcept
    {
        size_t i;
        for (i = 0; i < packets.size(); ++i)
        {
            auto& packet = *packets[i];
            auto& mh = mmh[i].msg_hdr;
            mh.msg_name         = &packet.address;
            mh.msg_namelen      = sizeof(packet.address);
            mh.msg_iov          = &packet.io;
            mh.msg_iovlen       = 1;
            mh.msg_control      = nullptr;
            mh.msg_controllen   = 0;
            mh.msg_flags        = 0;
            packet.io.iov_base  = packet.data;
            packet.io.iov_len   = packet.size;

            if (i == MNSU)
            {
                return i;
            }
        }

        return i;
    }
public:
    void Push(std::unique_ptr<Packet> packet) noexcept
    {
        packets.push_back(std::move(packet));
    }

    void Send(int fd) noexcept
    {       
        size_t len = Setup();

        int send_len = sendmmsg(fd, mmh, len, MSG_DONTWAIT);

        for (int i = 0; i < send_len; ++i)
        {
            packets.pop_front();
        }
    }
};

int SEND_QUEUE_TEST()
{
    SendQueue sq;

    auto packet = std::make_unique<Packet>();
    *(uint8_t*)(packet->data) = 48;
    *(float*)(packet->data + sizeof(uint8_t)) = 535.0f;
    *(float*)(packet->data + sizeof(uint8_t) + sizeof(float)) = 48.8f;
    packet->size = sizeof(uint8_t) + sizeof(float) * 2;
    packet->address.sin_family = AF_INET;
    packet->address.sin_port = htons(4888);
    packet->address.sin_addr.s_addr = inet_addr("127.0.0.1");

    sq.Push(std::move(packet));

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    
    while (true)
        sq.Send(fd);

    return 0;
}

#endif

