
#ifndef PACKET_HPP
#define PACKET_HPP

#include <stddef.h>
#include <stdint.h>
#include <arpa/inet.h>

#define MTU 512

struct Packet
{
    iovec io;
    sockaddr_in address;
    uint8_t data[MTU];
    size_t size;
};

struct Packet256
{
    iovec io;
    sockaddr_in address;
    uint8_t data[UINT8_MAX];
    uint8_t byte_size;
};

#endif

