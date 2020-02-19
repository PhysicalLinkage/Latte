
#ifndef PACKET_HPP
#define PACKET_HPP

#include <stddef.h>
#include <stdint.h>
#include <arpa/inet.h>

#include <memory>
#include <vector>

#define MTU 512

struct Packet
{
    sockaddr_in address;
    uint8_t data[MTU];
    size_t size;
};

struct Packet256
{
    sockaddr_in address;
    uint8_t data[UINT8_MAX];
    uint8_t byte_size;
};

#endif
