
#ifndef MUDP_HPP
#define MUDP_HPP

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>

#define CLIENT_PORT 0

template<uint16_t PORT>
struct MUDP
{
    const int           socket_fd;

    explicit MUDP() noexcept
        : socket_fd {socket(AF_INET, SOCK_DGRAM|SOCK_NONBLOCK, 0)}
    {
        if (socket_fd == -1)
        {
            perror("socket");
            exit(1);
        }

        if (PORT == CLIENT_PORT)
        {
            return;
        }

        sockaddr_in addr;
        addr.sin_family         = AF_INET;
        addr.sin_port           = htons(PORT);
        addr.sin_addr.s_addr    = INADDR_ANY;

        if (bind(socket_fd, (sockaddr*)&addr, sizeof(addr)) == -1)
        {
            perror("bind");
            exit(1);
        }

    }

    ~MUDP() noexcept
    {
        close(socket_fd);
    }

    int Recvmmsg(mmsghdr* mmsghdrs, size_t size) noexcept
    {
        return recvmmsg(socket_fd, mmsghdrs, size, MSG_DONTWAIT, 0);
    }

    int Sendmmsg(mmsghdr* mmsghdrs, size_t size) noexcept
    {
        return sendmmsg(socket_fd, mmsghdrs, size, MSG_DONTWAIT);
    }
};



static int MUDP_TEST()
{
    MUDP<53548> mudp;

    struct packet
    {
        uint8_t id;
        float x;
        float y;
    };

    constexpr size_t MNU = 10;
    constexpr size_t IOV_LEN = 3;
    constexpr size_t MMU = 512;

    mmsghdr mmhs[MNU];
    sockaddr_in addrs[MNU];
    iovec iovs[MNU][IOV_LEN];
    packet packets[MNU];
    

    for (size_t i = 0; i < MNU; ++i)
    {
        auto& msg_hdr       = mmhs[i].msg_hdr;
        msg_hdr.msg_name    = &addrs[i];
        msg_hdr.msg_namelen = sizeof(sockaddr_in);
        auto& iov           = iovs[i];
        msg_hdr.msg_iov     = iovs[i];
        msg_hdr.msg_iovlen  = IOV_LEN;
        auto& packet        = packets[i];
        iov[0].iov_base     = &packet.id;
        iov[0].iov_len      = sizeof(packet.id);
        iov[1].iov_base     = &packet.x;
        iov[1].iov_len      = sizeof(packet.x);
        iov[2].iov_base     = &packet.y;
        iov[2].iov_len      = sizeof(packet.y);
    }

    while (true)
    {
        int recv_size = mudp.Recvmmsg(mmhs, MNU);
        if (recv_size > 0)
        {
            for (int i = 0; i < recv_size; ++i)
            {
                auto& p = packets[i];
                printf("id      : %d\n", p.id);
                printf("(x, y)  : (%f, %f)\n", p.x, p.y);
            }
            mudp.Sendmmsg(mmhs, recv_size);
        }
    }
    

    mmsghdr send_mmh[MNU];


    return 0;
}

#endif



















