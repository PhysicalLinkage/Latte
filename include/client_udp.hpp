
#ifndef CLIENT_UDP_HPP
#define CLIENT_UDP_HPP

#include <arpa/inet.h>

#include <memory>
#include <vector>
#include <deque>

#include <base_udp.hpp>

static constexpr size_t UDP_CLIENT_MMRU = 8;
static constexpr size_t UDP_CLIENT_MMSU = 8;

namespace Client
{

struct RecvPacket
{
    std::unique_ptr<Message> message;
    iovec iov;

    void SetMessageSize(size_t size) noexcept
    {
        message->size = size;
    }

    void SetToMessageHeader(msghdr& msg_hdr) noexcept
    {
        message                 = std::make_unique<Message>();
        iov.iov_base            = message->data;
        iov.iov_len             = UDP_MTU;
        msg_hdr.msg_name        = NULL;
        msg_hdr.msg_namelen     = 0;
        msg_hdr.msg_iov         = &iov;
        msg_hdr.msg_iovlen      = 1;
        msg_hdr.msg_control     = NULL;
        msg_hdr.msg_controllen  = 0;
        msg_hdr.msg_flags       = 0;
    }
};

struct SendPacket
{
    std::unique_ptr<std::vector<iovec>> iovs;

    explicit SendPacket() noexcept {}

    explicit SendPacket(std::unique_ptr<std::vector<iovec>>&& iovs_) noexcept
        : iovs {std::move(iovs_)}
    {}

    void SetToMessageHeader(msghdr& msg_hdr) noexcept
    {
        msg_hdr.msg_name        = NULL;
        msg_hdr.msg_namelen     = 0;
        msg_hdr.msg_iov         = iovs->data();
        msg_hdr.msg_iovlen      = iovs->size();
        msg_hdr.msg_control     = NULL;
        msg_hdr.msg_controllen  = 0;
        msg_hdr.msg_flags       = 0;
    }
};

using BUDP = BaseUDP<RecvPacket, SendPacket, UDP_CLIENT_MMRU, UDP_CLIENT_MMSU>;

class UDP : public BUDP
{
public:
    explicit UDP(uint16_t port, const char* address) noexcept
        : BUDP {}
    {
        sockaddr_in addr        = {0};
        addr.sin_family         = AF_INET;
        addr.sin_port           = htons(port);
        addr.sin_addr.s_addr    = inet_addr(address);
        if (connect(socket_fd, (sockaddr*)&addr, sizeof(addr)) == -1)
        {
            perror("connect");
            exit(1);
        }
    }

protected:
    virtual void OnRecv(std::unique_ptr<Message>&& message) = 0;
    void OnRecv(RecvPacket& packet) noexcept override
    {
        OnRecv(std::move(packet.message));
    }
};

}

#endif

