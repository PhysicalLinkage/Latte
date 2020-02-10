
#ifndef LOGIN_HPP
#define LOGIN_HPP

#include <memory>
#include <packet.hpp>
#include <type_rudps.hpp>
#include <dhl.hpp>
#include <send_queue.hpp>

class Login
{
private:
    DHL dhl;   
    bool OkRequest;
    bool OkReply;
    bool OkAuthorization;
public:
    void OnRequest(std::unique_ptr<Packet> packet, SendQueue& send_queue) noexcept
    {
        if (packet->size != DHL_KEY_SIZE)
        {
            return;
        }

                
        dhl.GenerateKey(
    }
};


#endif

