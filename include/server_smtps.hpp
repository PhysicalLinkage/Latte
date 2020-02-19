
#ifndef SERVER_SMTPS_HPP
#define SERVER_SMTPS_HPP

#include <cmac.hpp>
#include <dhl.hpp>
#include <frame_timer.hpp>
#include <smtps.hpp>

#include <unordered_map>

class MySMTPS : public SMTPS
{
public:
    void OnRecv(std::unique_ptr<iovec>&& iov) noexcept override
    {
        printf("MySMTPS OnRecv\n");
    }
};

class ServerSMTPS : public UDPServer
{
public:
    explicit ServerSMTPS(uint16_t port) noexcept
        : UDPServer {port}
        , smtps {}
    {
        static uint8_t key[] = "123456789012345";
        smtps[inet_addr("127.0.0.1")][1].Setup(5e9, key);
    }
protected:
    void OnRecv(std::unique_ptr<sockaddr_in>&& address, std::unique_ptr<Packet>&& packet) noexcept override
    {
        smtps[address->sin_addr.s_addr][1].UpdateRecv(cmac, packet->data + 1, packet->size - 1);
    }

    void Update() noexcept
    {
        RecvUpdate();

        for (auto& pairs : smtps)
        {
            for (auto& pair : pairs.second)
            {
                auto& value = pair.second;
                value.UpdateSend(cmac, Send);
            }
        }

        SendUpdate();
    }
private:
    std::unordered_map<uint32_t, std::unordered_map<uint16_t, MySMTPS>> smtps;
    CMAC cmac;
};

#endif

