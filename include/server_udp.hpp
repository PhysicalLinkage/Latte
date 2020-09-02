
#ifndef SERVER_UDP_HPP
#define SERVER_UDP_HPP

#include <arpa/inet.h>

#include <memory>
#include <vector>
#include <deque>

#include <client_udp.hpp>

#define UDP_SERVER_MMRU 100
#define UDP_SERVER_MMSU 100

namespace Server
{

struct RecvPacket
{
    std::unique_ptr<sockaddr_in> address;
    std::unique_ptr<Message> message;
    iovec iov;

    void SetMessageSize(size_t size) noexcept
    {
        message->size = size;
    }

    void SetToMessageHeader(msghdr& msg_hdr) noexcept
    {
        constexpr size_t size_of_sockaddr_in = sizeof(sockaddr_in);
        
        address                 = std::make_unique<sockaddr_in>();
        message                 = std::make_unique<Message>();
        iov.iov_base            = message->data;
        iov.iov_len             = UDP_MTU;
        msg_hdr.msg_name        = address.get();
        msg_hdr.msg_namelen     = size_of_sockaddr_in;
        msg_hdr.msg_iov         = &iov;
        msg_hdr.msg_iovlen      = 1;
        msg_hdr.msg_control     = NULL;
        msg_hdr.msg_controllen  = 0;
        msg_hdr.msg_flags       = 0;
    }
};

struct SendPacket
{
    sockaddr_in& address;
    std::unique_ptr<std::vector<iovec>> iovs;

    explicit SendPacket(sockaddr_in& address_, std::unique_ptr<std::vector<iovec>> iovs_) noexcept
        : address {address_}
        , iovs {std::move(iovs_)}
    {
    }

    void SetToMessageHeader(msghdr& msg_hdr) noexcept
    {
        constexpr size_t size_of_sockaddr_in = sizeof(sockaddr_in);
        
        msg_hdr.msg_name        = &address;
        msg_hdr.msg_namelen     = size_of_sockaddr_in;
        msg_hdr.msg_iov         = iovs->data();
        msg_hdr.msg_iovlen      = iovs->size();
        msg_hdr.msg_control     = NULL;
        msg_hdr.msg_controllen  = 0;
        msg_hdr.msg_flags       = 0;
    }
};

using BUDP = BaseUDP<RecvPacket, SendPacket, UDP_SERVER_MMRU, UDP_SERVER_MMSU>;

class UDP : public BUDP
{
public:
    explicit UDP(uint16_t port) noexcept
        : BUDP {}
    {
        sockaddr_in addr = {0};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = INADDR_ANY;
        if (bind(socket_fd, (sockaddr*)&addr, sizeof(addr)) == -1)
        {
            perror("bind");
            exit(1);
        }
    }

protected:
    virtual void OnRecv(
            std::unique_ptr<sockaddr_in>&& address,
            std::unique_ptr<Message>&& message) = 0;
    void OnRecv(RecvPacket& packet) noexcept override
    {
        
        OnRecv(std::move(packet.address), std::move(packet.message));
    }
};

}

#endif

