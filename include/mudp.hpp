
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
    const int socket_fd;

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



int MUDP_TEST()
{
    MUDP<4888> mudp;


    constexpr size_t MNU = 4;

    mmsghdr mmhs[MNU];
    sockaddr_in addrs[MNU];
    iovec iovs[MNU];
    uint8_t data[512];
    

    for (size_t i = 0; i < MNU; ++i)
    {
        auto& msg_hdr       = mmhs[i].msg_hdr;
        msg_hdr.msg_name    = &addrs[i];
        msg_hdr.msg_namelen = sizeof(sockaddr_in);
        auto& iov           = iovs[i];
        msg_hdr.msg_iov     = &iov;
        msg_hdr.msg_iovlen  = 1;
        iov.iov_base        = &data;
        iov.iov_len         = sizeof(data);
    }

    while (true)
    {
        int recv_size = mudp.Recvmmsg(mmhs, MNU);
        if (recv_size > 0)
        {
            printf("recv\n");
            for (int i = 0; i < recv_size; ++i)
            {
                size_t size = mmhs[i].msg_len;
                printf("packet size : %u\n", size);
                printf("packet data : %s\n", data);
                iovs[i].iov_len = size;
            }
            mudp.Sendmmsg(mmhs, recv_size);
        }
    }

    return 0;
}

#endif



















