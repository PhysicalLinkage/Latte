
#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <mudp.hpp>
#include <cmac.hpp>
#include <turn_packet.hpp>

class Client
{
    uint8_t key[CMAC_KEY_SIZE];
    sockaddr_in address;
    uint16_t recv_seq;
    uint16_t recv_ack;
    uint16_t send_seq;
    TurnPacket<512> 
}

int CLIENT_TEST()
{
    MUDP<CLIENT_PORT> mudp;

    msghdr msg_hdr = {0};
    sockaddr_in addr = {0};
    iovec iovecs[TURN_ELEMENT_SIZE] = {0};

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
    memcpy(packet.data, "ok", 3);

    packet.SetToIovecs(iovecs, 3);
    
    CMAC cmac;

    if (!packet.TrySetMac(cmac, key, 3))
    {
        return 0;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(4888);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    sendmsg(mudp.socket_fd, &msg_hdr, MSG_DONTWAIT);
    perror("sendmmsg");

    return 0;
}

#endif

