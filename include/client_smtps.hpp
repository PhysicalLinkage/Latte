
#ifndef CLIENT_SMTPS_HPP
#define CLIENT_SMTPS_HPP

#include <arpa/inet.h>

#include <dhl.hpp>


struct ClientSMTPS
{
    int fd;
    sockaddr_in addr;
    uint8_t key[DHL_KEY_SIZE];

    explicit ClientSMTPS(uint16_t port, const char* address) noexcept
        : fd {socket(AF_INET, SOCK_DGRAM, 0)}
        , addr {}
    {
        if (fd == -1)
        {
            perror("socket");
            exit(1);
        }

        sockaddr_in addr = {0};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(4888);
        addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        if (connect(fd, (sockaddr*)&addr, sizeof(addr)) == -1)
        {
            perror("connect");
            exit(1);
        }
    }
};


#endif
