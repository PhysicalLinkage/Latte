
#ifndef UDP_CLIENT
#define UDP_CLIENT

#include <arpa/inet.h>

#include <memory>
#include <vector>
#include <list>
#include <deque>

#include <udp_def.hpp>

#define UDP_CLIENT_MMRU 8
#define UDP_CLIENT_MMSU 8

class UDPClient
{
public:
    struct Packet
    {   
        uint8_t data[UDP_MTU];
        uint16_t size;
    };

private:
    struct RecvPacket
    {
    public:
        void SetupMessageHeader(msghdr& msg_hdr) noexcept
        {
            packet = std::make_unique<Packet>();
            iov.iov_base = packet->data;
            iov.iov_len  = sizeof(packet->data);
            msg_hdr.msg_name        = NULL;
            msg_hdr.msg_namelen     = 0;
            msg_hdr.msg_iov         = &iov;
            msg_hdr.msg_iovlen      = 1;
            msg_hdr.msg_control     = NULL;
            msg_hdr.msg_controllen  = 0;
            msg_hdr.msg_flags       = 0;
        }

        void SetupPacket(mmsghdr& mmsghdr) noexcept
        {
            packet->size = mmsghdr.msg_len;
        }
        
        std::unique_ptr<Packet>&& Pop() noexcept
        {   
            return std::move(packet);
        }
        
        std::unique_ptr<Packet> packet;
    private:
        iovec iov;
    };
public:
    struct SendPacket
    {
    public:
        explicit SendPacket(
            std::unique_ptr<std::vector<iovec>>&& iovs_) noexcept
            : iovs {std::move(iovs_)}
        {
        }

        void SetupMessageHeader(msghdr& msg_hdr) noexcept
        {
            msg_hdr.msg_name        = NULL;
            msg_hdr.msg_namelen     = 0;
            msg_hdr.msg_iov         = iovs->data();
            msg_hdr.msg_iovlen      = iovs->size();
            msg_hdr.msg_control     = NULL;
            msg_hdr.msg_controllen  = 0;
            msg_hdr.msg_flags       = 0;
        }

    private:
        std::unique_ptr<std::vector<iovec>> iovs;
    };
public:
    explicit UDPClient(uint16_t port, const char* address) noexcept
        : Send {*this}
        , socket_fd {socket(AF_INET, SOCK_DGRAM, 0)}
        , recv_mmhs {}
        , recv_packets {}
        , send_mmhs {}
    {
        if (socket_fd == -1)
        {
            perror("socket");
            exit(1);
        }

        sockaddr_in addr = {0};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = inet_addr(address);
        if (connect(socket_fd, (sockaddr*)&addr, sizeof(addr)) == -1)
        {
            perror("connect");
            exit(1);
        }

        for (size_t i = 0; i < UDP_CLIENT_MMRU; ++i)
        {
            recv_packets[i].SetupMessageHeader(recv_mmhs[i].msg_hdr);
        }
    }

    void RecvUpdate() noexcept
    {
        const int recv_result = recvmmsg(socket_fd, recv_mmhs, UDP_CLIENT_MMRU, MSG_DONTWAIT, NULL);
        if (recv_result > 0)
        {
            printf("recv\n");
            for (int i = 0; i < recv_result; ++i)
            {
                recv_packets[i].SetupPacket(recv_mmhs[i]);
                OnRecv(std::move(recv_packets[i].Pop()));
                recv_packets[i].SetupMessageHeader(recv_mmhs[i].msg_hdr);
            }
        }
    }

    void SendUpdate() noexcept
    {
        const size_t send_packets_size = send_packets.size();
        const size_t send_size = (send_packets_size < UDP_CLIENT_MMSU) ? send_packets_size : UDP_CLIENT_MMSU;
        for (size_t i = 0; i < send_size; ++i)
        {
            send_packets[i]->SetupMessageHeader(send_mmhs[i].msg_hdr);
        }
        const int send_result = sendmmsg(socket_fd, send_mmhs, send_size, MSG_DONTWAIT);
        for (int i = 0; i < send_result; ++i)
        {
            send_packets.pop_front();
        }
    }
protected:
    virtual void OnRecv(std::unique_ptr<Packet>&& packets_) = 0;
public:
    class SendMethod
    {
    public:
        explicit SendMethod(UDPClient& client_) noexcept
            : client {client_}
        {
        }
        void operator()(std::unique_ptr<SendPacket>&& send_packet) noexcept
        {
            client.send_packets.push_back(std::move(send_packet));
        }
    private:
        UDPClient& client;
    };
    friend class SendMethod;
    SendMethod Send;

private:
    const int socket_fd;
    mmsghdr recv_mmhs[UDP_CLIENT_MMRU];
    RecvPacket recv_packets[UDP_CLIENT_MMRU];
    mmsghdr send_mmhs[UDP_CLIENT_MMSU];
    std::deque<std::unique_ptr<SendPacket>> send_packets;
};

#endif

