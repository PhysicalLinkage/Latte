
#ifndef RUDPS_HPP
#define RUDPS_HPP

#include <list>
#include <deque>
#include <unordered_map>

#include <ftm.hpp>
#include <source.hpp>
#include <mudp.hpp>
#include <cmac.hpp>

namespace Latte
{

namespace Linux
{

namespace Network
{

template<size_t MMU>
struct Packet
{
    sockaddr_in addr;
    std::array<uint8_t, MMU> array;
};

template<size_t MMU>
struct User
{
    uint8_t key[CMAC::KEY_SIZE];
    uint16_t id;
    timespec time;
    uint8_t seq;
    uint8_t ack;
    sockaddr_in addr;
    std::deque<std::array<uint8_t, MMU>> messages;
};

template<size_t PORT, size_t MNRU, size_t MNSU, size_t MMU>
struct RUDPS
{
    static constexpr size_t HEADER_SIZE = 
        sizeof(uint8_t) * CMAC::MAC_SIZE + 
        sizeof(uint64_t) +
        sizeof(uint16_t) * 3;

    MUDP<PORT, MNRU, MNSU> mudp;
    sockaddr_in recv_addrs[MNRU];
    Packet<MMU> recv_msgs[MNRU];
    std::deque<sockaddr_in> send_addrs;
    std::deque<Packet<MMU>> send_msgs;
    std::unordered_map<uint16_t, User<MMU>> users;
    CMAC cmac;
    timespec now_time;
    
    explicit RUDPS() noexcept
        : mudp {}
        , recv_addrs {}
        , recv_msgs {}
        , send_addrs {}
        , send_msgs {}
        , users {}
        , cmac {}
        , now_time {}
    {
        for (size_t i = 0; i < MNRU; ++i)
        {
            mudp.recv_mmhs[i].msg_hdr.msg_name = &recv_addrs[i];
            mudp.recv_mmhs[i].msg_hdr.msg_namelen = sizeof(sockaddr_in);
            mudp.recv_iovecs[i].iov_base = recv_msgs[i].array.data();
            mudp.recv_iovecs[i].iov_len = recv_msgs[i].array.size();
        }
    }

    void Run() noexcept
    {
        while (true)
        {
            clock_gettime(CLOCK_REALTIME_COARSE, &now_time);

            for (size_t i = 0; i < users.size(); ++i)
            {
                if ((now_time.tv_sec - users[i].time.tv_sec) > 4)
                    users.erase(i);
            }

            int recv_num = mudp.Recvmmsg();
            if (recv_num > 0)
                Update(recv_num);

            for (size_t i = 0; i < MNSU; ++i)
            {
                mudp.send_mmhs[i].msg_hdr.msg_name = &send_addrs[i];
                mudp.send_mmhs[i].msg_hdr.msg_namelen = sizeof(sockaddr_in);
                mudp.send_iovecs[i].iov_base = send_msgs[i].array.data();
                mudp.send_iovecs[i].iov_len = send_msgs[i].array.size();
            }

            int send_num = mudp.Sendmmsg();
            if (send_num > 0)
                for (size_t i = 0; i < send_num; ++i)
                    send_msgs.pop_front();
        }
    }

    void Update(int recv_num) noexcept
    {
        for (size_t i = 0; i < recv_num; ++i)
        {
            auto& recv_addr = mudp.recv_mmhs[i].msg_hdr.msg_name;
            auto* recv_data = (uint8_t*)mudp.recv_iovecs[i].iov_base;
            auto& recv_size = mudp.recv_mmhs[i].msg_len;

            if (recv_size < HEADER_SIZE)
                break;

            FTM::Deserializer dese(recv_data, recv_size);
            
            // Authentication
            const uint8_t* recv_mac;
            uint16_t recv_id;
            if ((!dese.Bytes(&recv_mac, CMAC::MAC_SIZE)) ||
                (!dese.To(recv_id)))
                break;

            if (users.count(recv_id) == 0)
                break;

            auto& user = users[recv_id];

            if (!cmac.Init(user.key))
            {
                break;
            }

            if (!cmac.Update(&(recv_data[CMAC::MAC_SIZE]), recv_size - CMAC::MAC_SIZE))
                break;

            uint8_t mac[CMAC::MAC_SIZE];
            if (!cmac.Final(mac))
                break;

            if (memcmp(mac, recv_mac, CMAC::MAC_SIZE) != 0)
                break;


            // Reliable
            uint16_t recv_seq;
            uint16_t recv_ack;
            if  ((!dese.To(recv_seq)) ||
                (!dese.To(recv_ack)) ||
                (!dese.forward(sizeof(uint64_t))))
                break;

            std::list<uint8_t> sizes;
            std::list<const uint8_t*> messages;

            uint16_t seq_i;
            for (seq_i = user.seq; i < recv_seq; ++i)
            {
                uint8_t size;
                const uint8_t* message;
                if ((!dese.To(size)) ||
                    (!dese.Bytes(&message, size)))
                    break;
                
                sizes.push_back(size);
                messages.push_back(message);
            }
            if (seq_i != recv_seq)
                break;

            uint16_t ack_distance = (user.ack <= recv_ack) ?
                recv_ack - user.ack :
                user.ack - recv_ack ;

            uint16_t ack_i;
            for (ack_i = 0; ack_i < ack_distance; ++ack_i)
            {
                if (user.messages.empty())
                {
                    users.erase(recv_id);
                    break;
                }
                user.messages.pop_back();
            }
            if (ack_i != ack_distance)
                break;

            user.time = now_time;
            user.seq = recv_seq;
            user.ack = recv_ack;
            
            while (!messages.empty())
            {
                Receive(user, messages.back(), sizes.back());
                messages.pop_back();
                sizes.pop_back();
            }

            Packet<MMU> packet;
            packet.addr = user.addr;
            FTM::Serializer seri(packet.array.data());
            for (auto& message : user.messages)
            {
                seri.AddBytes(message.data(), message.size());
            }
            send_msgs.push_back(packet);
        }
    }

    void Receive(User<MMU>& user, const uint8_t* message, uint8_t size) noexcept
    {
        
    }

    void Send(User<MMU>& user, const uint8_t* message, uint8_t size) noexcept
    {
        std::array<uint8_t, MMU> array;
        memcpy(array.data(), message, size);
        user.messages.push_front(array);
    }

};

}

}

}

int RUDPS_TEST()
{
    using namespace Latte::Linux::Network;
    RUDPS<53548, 1000, 1000, 1000> rudps;
    rudps.Run();
    return 0;
}

#endif

