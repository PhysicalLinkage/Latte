
#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <mudp.hpp>
#include <cmac.hpp>
#include <turn_packet.hpp>

static int CLIENT_TEST()
{
    MUDP<CLIENT_PORT> mudp;

    msghdr msg_hdr;
    sockaddr_in addr;
    iovec iovecs[TURN_ELEMENT_SIZE];

    msg_hdr.msg_name    = (sockaddr*)&addr;
    msg_hdr.msg_namelen = sizeof(sockaddr_in);
    msg_hdr.msg_iov     = iovecs;
    msg_hdr.msg_iovlen  = TURN_ELEMENT_SIZE;

    uint8_t key[CMAC_KEY_SIZE];

    for (uint8_t i = 0; i < CMAC_KEY_SIZE; ++i)
    {
        key[i] = i;
    }

    TurnPacket<512> packet;
    packet.id = 0;
    packet.seq = 1;
    packet.ack = 2;
    packet.x = 3;
    packet.y = 4;
    packet.z = 4.8;
    memcpy(packet.data, "ok", 3);

    packet.SetToIovecs(iovecs, 3);
    
    CMAC cmac;

    if (!packet.TrySetMac(cmac, key, 3))
    {
        return 0;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(53548);
    addr.sin_addr.s_addr = inet_addr("153.148.31.174");

    sendmsg(mudp.socket_fd, &msg_hdr, 0);
    perror("sendmmsg");

    return 0;
}

#endif

