
#ifndef RECV_QUEUE_HPP
#define RECV_QUEUE_HPP

#include <deque>
#include <memory>

#include <packet.hpp>


#define MNRU 100

class RecvQueue
{
private:
    mmsghdr mmhs[MNRU];
    std::deque<std::unique_ptr<Packet>> packets;
    int size;
public:
    explicit RecvQueue() noexcept
        : mmhs {0}
        , packets {}
        , size {0}
    {

    }

    bool Empty() noexcept
    {
        return size == 0;
    }

    std::unique_ptr<Packet> Pop() noexcept
    {
        auto packet = std::move(packets.front());
        packets.pop_front();
        --size;
        return std::move(packet);
    }

    void Recv(int fd) noexcept
    {
        for (size_t i = packets.size(); i < MNRU; ++i)
        {
            auto& mh = mmhs[i].msg_hdr;
            auto packet = std::make_unique<Packet>();
            mh.msg_name         = &packet->address;
            mh.msg_namelen      = sizeof(packet->address);
            mh.msg_iov          = &packet->io;
            mh.msg_iovlen       = 1;
            mh.msg_control      = nullptr;
            mh.msg_controllen   = 0;
            mh.msg_flags        = 0;
            packet->io.iov_base = packet->data;
            packet->io.iov_len  = packet->size;
            packets.push_back(std::make_unique<Packet>());
        }
    
        size = recvmmsg(fd, mmhs, MNRU, MSG_DONTWAIT, 0);

        for (int i = 0; i < size; ++i)
        {
            packets[i]->size = mmhs[i].msg_len;
        }
    }
};

int RECV_QUEUE_TEST()
{
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(4888);
    addr.sin_addr.s_addr = INADDR_ANY;
    bind(fd, (sockaddr*)&addr, sizeof(sockaddr_in));

    RecvQueue rq;

    rq.Empty();
    return 0;
    while (true)
    {
        rq.Recv(fd);
        while (!rq.Empty())
        {
            auto packet = std::move(rq.Pop());
            printf("id  = %d\n", *(uint8_t*)(packet->data));
            printf("len = %lu\n", packet->size);
        }
    }
    return 0;
}

#endif

