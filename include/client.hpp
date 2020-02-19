
#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <udp_client.hpp>

class Client : public UDPClient
{
public:
    explicit Client() noexcept
        : UDPClient {4888, "127.0.0.1"}
    {
    }

    void OnRecv(std::unique_ptr<Packet>&& packet_) noexcept override;
    void Update() noexcept;
};

#endif

