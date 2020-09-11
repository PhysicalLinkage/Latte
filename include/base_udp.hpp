
#ifndef BASE_UDP_HPP
#define BASE_UDP_HPP

#include <arpa/inet.h>

#include <memory>
#include <array>
#include <vector>
#include <deque>

static constexpr size_t UDP_MTU = 512;

/*
struct RecvPacket
{
    virtual void SetMessageSize(size_t size) = 0;
    virtual void SetToMessageHeader(msghdr& msg_hdr) = 0;
};

struct SendPacket
{
    virtual void SetToMessageHeader(msghdr& msg_dhr) = 0;
};
*/

struct Message
{
    uint8_t data[UDP_MTU];
    uint16_t size;
};

template<class RecvPacket, class SendPacket, size_t MMRU, size_t MMSU>
class BaseUDP
{
public:
    explicit BaseUDP() noexcept
        : socket_fd {socket(AF_INET, SOCK_DGRAM, 0)}
        , recv_mmhs {}
        , recv_packets {}
        , send_mmhs {}
        , send_packets {}
    {
        if (socket_fd == -1)
        {
            perror("socket");
            exit(1);
        }

        for(size_t i = 0; i < MMRU; ++i)
        {
            recv_packets[i].SetToMessageHeader(recv_mmhs[i].msg_hdr);
        }
    }

    void RecvUpdate() noexcept
    {
        const int recv_result = recvmmsg(socket_fd, recv_mmhs, MMRU, MSG_DONTWAIT, NULL);
        if (recv_result > 0)
        {
            for (int i = 0; i < recv_result; ++i)
            {
                recv_packets[i].SetMessageSize(recv_mmhs[i].msg_len);
                OnRecv(recv_packets[i]);
                recv_packets[i].SetToMessageHeader(recv_mmhs[i].msg_hdr);
            }
        }
    }

    void SendUpdate() noexcept
    {
        const size_t send_packets_size = send_packets.size();
        const size_t send_size = (send_packets_size < MMSU) ? send_packets_size : MMSU;
        for (size_t i = 0; i < send_size; ++i)
        {
            send_packets[i]->SetToMessageHeader(send_mmhs[i].msg_hdr);
        }
        sendmmsg(socket_fd, send_mmhs, send_size, MSG_DONTWAIT);
        for (size_t i = 0; i < send_size; ++i)
        {
            send_packets.pop_front();
        }
        
    }

    void Send(std::unique_ptr<SendPacket>&& send_packet) noexcept
    {
        send_packets.push_back(std::move(send_packet));
    }
protected:
    virtual void OnRecv(RecvPacket&) = 0;
    
    const int socket_fd;
    mmsghdr recv_mmhs[MMRU];
    RecvPacket recv_packets[MMRU];
    mmsghdr send_mmhs[MMSU];
    std::deque<std::unique_ptr<SendPacket>> send_packets;
};



#endif

