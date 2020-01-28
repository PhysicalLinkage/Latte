
#ifndef OLD_MUDP_HPP
#define OLD_MUDP_HPP

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <memory>

#include <netinet/in.h>
#include <arpa/inet.h>

namespace Latte
{

namespace Linux
{

namespace Network
{

class IPEndpoint
{
private:
    struct sockaddr_in addr;
public:
    explicit IPEndpoint() noexcept
        : addr {}
    {
        addr.sin_family = AF_INET;
    }

    explicit IPEndpoint(uint16_t port, uint32_t address) noexcept
        : addr {}
    {
        addr.sin_family         = AF_INET;
        addr.sin_port           = port;
        addr.sin_addr.s_addr    = address;
    }

    explicit IPEndpoint(const IPEndpoint& other) = delete;
    IPEndpoint& operator=(const IPEndpoint& other) noexcept
    {
        Port() = other.Port();
        Address() = other.Address();
    }

    struct sockaddr_in& SockaddrIN() noexcept { return addr; }

    const uint16_t& Port()      const noexcept { return addr.sin_port; }
    const uint32_t& Address()   const noexcept { return addr.sin_addr.s_addr; }

    uint16_t& Port()    noexcept { return addr.sin_port; }
    uint32_t& Address() noexcept { return addr.sin_addr.s_addr; }

    const char * AddressAsString() const noexcept { return inet_ntoa(addr.sin_addr); } 

    bool operator==(const IPEndpoint& other) const noexcept
    {
        return (Port() == other.Port() && Address() == other.Address());
    }

    bool operator!=(const IPEndpoint& other) const noexcept
    {
        return (Port() != other.Port() || Address() != other.Address());
    }
};


template<size_t MAXMUM_MESSAGE_UNIT = 512>
class IPPacket
{
private:
    struct mmsghdr&     mmsghdr;
    IPEndpoint          endpoint;
    struct iovec        iovec;
    uint8_t             message[MAXMUM_MESSAGE_UNIT];

public:
    explicit IPPacket(struct mmsghdr& mmsghdr_) noexcept
        : mmsghdr   {mmsghdr_}
        , endpoint  {}
        , iovec     {}
        , message   {}
    {
        mmsghdr.msg_hdr.msg_name    = &endpoint.SockaddrIN();
        mmsghdr.msg_hdr.msg_namelen = sizeof(endpoint.SockaddrIN());
        mmsghdr.msg_hdr.msg_iov     = &iovec;
        mmsghdr.msg_hdr.msg_iovlen  = 1;
        iovec.iov_base              = message;
        iovec.iov_len               = MAXMUM_MESSAGE_UNIT;
    }

    explicit IPPacket(const IPPacket&) = delete;
    IPPacket& operator=(const IPPacket& other) noexcept
    {
        Endpoint()  = other.Endpoint();
        Size()      = other.Size();
        memcpy(Message(), other.Message(), other.Size());
    }

    const IPEndpoint&   Endpoint() const noexcept { return endpoint; }
    const uint32_t&     Size    () const noexcept { return mmsghdr.msg_len; }
    const uint8_t*      Message () const noexcept { return message; }

    IPEndpoint& Endpoint() noexcept { return endpoint; }
    size_t&     Size    () noexcept { return iovec.iov_len; }
    uint8_t*    Message () noexcept { return message; }
};


template<size_t MAXMUM_NUMBER_MESSAGE_UNIT = 128, size_t MAXMUM_MESSAGE_UNIT = 512>
class MultipleMessageHeaders
{
    const int&      socket_fd;
    struct mmsghdr  mmsghdrs[MAXMUM_NUMBER_MESSAGE_UNIT];
    std::unique_ptr<IPPacket<MAXMUM_MESSAGE_UNIT>> packets[MAXMUM_NUMBER_MESSAGE_UNIT];

public:
    explicit MultipleMessageHeaders(const int& socket_fd_) noexcept
        : socket_fd {socket_fd_}
        , mmsghdrs  {}
        , packets   {}
    {
        for (size_t i = 0; i < MAXMUM_NUMBER_MESSAGE_UNIT; ++i)
        {
            packets[i].reset(new IPPacket<MAXMUM_MESSAGE_UNIT>(mmsghdrs[i]));
        }
    }

    const IPPacket<MAXMUM_MESSAGE_UNIT>&   Packet(size_t i) const  noexcept { return *packets[i]; }
    IPPacket<MAXMUM_MESSAGE_UNIT>&         Packet(size_t i)        noexcept { return *packets[i]; }

    int Sendmmsg(size_t i, size_t size) noexcept
    {
        return sendmmsg(socket_fd, &mmsghdrs[i], size, MSG_DONTWAIT);
    }

    int Recvmmsg() noexcept
    {
        return recvmmsg(socket_fd, mmsghdrs, MAXMUM_NUMBER_MESSAGE_UNIT, MSG_DONTWAIT, 0);
    }

};

constexpr size_t pow2(size_t n, size_t result = 1)
{
    return (n == 0) ? result : pow2(n - 1, result << 1);
}

constexpr size_t log2(size_t x, size_t result = 0)
{
    return (x == 1) ? result : log2(x >> 1, result + 1);
}

#define CLIENT_PORT (0)

template<uint16_t PORT, size_t MAXMUM_NUMBER_SEND_PER_RECEIVE = 8, size_t MAXMUM_NUMBER_RECEIVE_UNIT = 128, size_t MAXMUM_MESSAGE_UNIT = 512>
class MUDP 
{
private:
    constexpr static size_t MAXMUM_NUMBER_SEND_UNIT = pow2(log2(MAXMUM_NUMBER_RECEIVE_UNIT * MAXMUM_NUMBER_SEND_PER_RECEIVE) + 1);
public:
    constexpr static uint16_t Port                  = PORT;
    constexpr static size_t MaxmumMessageUnit       = MAXMUM_MESSAGE_UNIT;
    constexpr static size_t MaxmumNumberSendUnit    = MAXMUM_NUMBER_SEND_UNIT;
    constexpr static size_t MaxmumNumberReceiveUnit = MAXMUM_NUMBER_RECEIVE_UNIT;
private:
    const int   socket_fd;
    int         send_begin;
    int         send_end;
    int         send_next;
    MultipleMessageHeaders<MAXMUM_NUMBER_SEND_UNIT, MAXMUM_MESSAGE_UNIT>    multiple_sender;
    MultipleMessageHeaders<MAXMUM_NUMBER_RECEIVE_UNIT, MAXMUM_MESSAGE_UNIT> multiple_receiver;

public:
    explicit MUDP() noexcept
        : socket_fd         {socket(AF_INET, SOCK_DGRAM|SOCK_NONBLOCK, 0)}
        , send_begin        {0}
        , send_end          {0}
        , send_next         {0}
        , multiple_sender   {socket_fd}
        , multiple_receiver {socket_fd}
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

        struct sockaddr_in addr;
        addr.sin_family         = AF_INET;
        addr.sin_port           = htons(PORT);
        addr.sin_addr.s_addr    = INADDR_ANY;

        if (bind(socket_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
        {
            perror("bind");
            exit(1);
        }
    }

    ~MUDP() noexcept
    {
        close(socket_fd);
    }


    IPPacket<MAXMUM_MESSAGE_UNIT>& SendedPacket() noexcept 
    {
        constexpr size_t prev_mnsu  = MAXMUM_NUMBER_SEND_UNIT - 1;
        const int prev_send_next    = send_next;
        send_next = (send_next + 1) & prev_mnsu;
        if (send_begin != send_next)
        {
            send_end += ((size_t)send_end < MAXMUM_NUMBER_SEND_UNIT) ? 1 : 0;
        }
        else
        {
            send_next = prev_send_next;
        }
        return multiple_sender.Packet(prev_send_next);
    }
    
    const IPPacket<MAXMUM_MESSAGE_UNIT>& ReceivedPacket(size_t i) const noexcept 
    { 
        return multiple_receiver.Packet(i); 
    }

    int MultipleSend() noexcept
    {   
        const int send_size   = send_end - send_begin;
        const int sended_size = multiple_sender.Sendmmsg(send_begin, send_size);
        if (sended_size > 0)
        {

            if (sended_size == send_size)
            {
                send_begin = 0;
                if (send_end != MAXMUM_NUMBER_SEND_UNIT)
                {
                    send_end    = 0;
                    send_next   = 0;
                }
                else
                {
                    send_end = send_next;
                }
            }
            else
            {
                send_begin  += sended_size;
            }
        }
        return sended_size;
    }

    int MultipleReceive() noexcept 
    {
        return multiple_receiver.Recvmmsg();
    }

};

}

}

}

int OLD_MUDP_TEST()
{
    using namespace Latte::Linux::Network;

    MUDP<53548> mudp;

    while (true)
    {
        const int received_number = mudp.MultipleReceive();
        if (received_number > 0)
        {
            printf("received_number : %d\n", received_number);

            for (int i = 0; i < received_number; ++i)
            {
                printf("    received index : %d\n", i);

                const auto& rp = mudp.ReceivedPacket(i);
                printf("        rp.Port()           : %d\n", rp.Endpoint().Port());
                printf("        rp.AddressAsString(): %s\n", rp.Endpoint().AddressAsString());
                printf("        rp.Size()           : %d\n", rp.Size());
                printf("        rp.Message()        : %s\n", rp.Message());
            
                auto& sp = mudp.SendedPacket();
                sp = rp;
            }
        }
        
        const int sended_number = mudp.MultipleSend();
        if (sended_number > 0)
        {
            printf("sended_number : %d\n\n", sended_number);
        }
    }
    return 0;
}

#endif

