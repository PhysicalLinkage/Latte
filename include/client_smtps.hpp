
#ifndef CLIENT_SMTPS_HPP
#define CLIENT_SMTPS_HPP

#include <arpa/inet.h>

#include <dhl.hpp>

#include <udp_client.hpp>

class ClientSMTPS : public UDPClient
{

public:
    explicit ClientSMTPS(uint16_t port, const char* address) noexcept
        : UDPClient {port, address}
    {
        Update = &ClientSMTPS::PushLoginSend;
    }
protected:
    void OnRecv(std::unique_ptr<Packet>&& packet_) noexcept
    {

    }
public:
    void PushLoginSend() noexcept
    {
    }

    void (ClientSMTPS::*Update)();
};


#endif
