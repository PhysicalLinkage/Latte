
#ifndef TURN_HPP
#define TURN_HPP

#include <mudp.hpp>
#include <turn_packet.hpp>
#include <flyweight.hpp>
#include <frame_timer.hpp>
#include <vector3.hpp>

#include <deque>
#include <vector>

template<uint16_t PORT, size_t MNCU = 4, size_t MNRU = 4, size_t MNSU = 4, size_t MMU = 512>
class Turn
{
public:
    using PoolIndexs = std::deque<size_t>;
    
    struct Data
    {
        uint16_t size;
        uint8_t* data;
    };

    struct Client
    {
        uint8_t key[CMAC_KEY_SIZE];
        sockaddr_in address;
        FrameTimer timer_for_using;
        FrameTimer timer_for_send;
        uint16_t seq;
        uint16_t ack;
        uint16_t send_seq;
        std::deque<Data> datas;
    };

    using Packet        = TurnPacket<MMU>;
    using AddresPool    = Flyweight<sockaddr_in>;
    using PacketPool    = Flyweight<Packet>;
    using ClientPool    = Flyweight<Client>;
private:
    MUDP<PORT> mudp;
    
    mmsghdr recv_mmhs[MNRU];
    iovec   recv_iovecss[MNRU][TURN_ELEMENT_SIZE];
    size_t  recv_addres_indexs[MNRU];
    size_t  recv_packet_indexs[MNRU];

    mmsghdr     send_mmhs[MNSU];
    iovec       send_iovecss[MNSU][TURN_ELEMENT_SIZE];
    PoolIndexs  send_addres_indexs;
    PoolIndexs  send_packet_indexs;

    AddresPool  addres_pool;
    PacketPool  packet_pool;
    ClientPool  client_pool;

    CMAC cmac;
    Frame frame;
public:

    explicit Turn() noexcept
        : mudp {}
        , recv_mmhs {}
        , recv_iovecss {}
        , recv_addres_indexs {}
        , recv_packet_indexs {}
        , send_mmhs {}
        , send_iovecss {}
        , send_addres_indexs {}
        , send_packet_indexs {}
        , addres_pool {MNSU << 1}
        , packet_pool {MNSU << 1}
        , client_pool {MNCU}
        , cmac {}
        , frame {}
    {
        size_t i = client_pool.New();
        auto& client = client_pool[i];
        client->timer_for_using.Setup(10e9);

        printf("client id : %lu\n", i);
        printf("client key: ");
        for (uint8_t i = 0; i < CMAC_KEY_SIZE; ++i)
        {
            printf("%x", i);
            client->key[i] = i;
        }
        printf("\n");
    }

    void Update() noexcept
    {
        frame.Update();

        UpdateClients();

        SetupRecvmmsg();

        int recv_size = mudp.Recvmmsg(recv_mmhs, MNRU);

        if (recv_size <= 0)
        {
            return;
        }

        printf("%d recv\n", recv_size); 

        for (int i = 0; i < recv_size; ++i)
        {
            auto& size = recv_mmhs[i].msg_len;
            
            if (size < TURN_HEADER_SIZE)
            {
                printf("less of size\n");
                continue;
            }

            auto& packet = packet_pool[recv_packet_indexs[i]];

            if (!client_pool.IsUsed(packet->id))
            {
                printf("!client_pool.IsUsed\n");
                continue;
            }

            auto& client = client_pool[packet->id];

            if (!packet->TryCheckMac(cmac, client->key, size))
            {
                printf("size    : %u\n", size);
                printf("data len: %lu\n", size - TURN_HEADER_SIZE);
                packet->DebugLog(stdout);
                continue;
            }

            printf("Success auth\n");

            client->timer_for_using.Reset();

            uint16_t offset = 0;
            for (uint16_t seq_i = packet->seq; seq_i !=  (client->seq - 1); --seq_i)
            {
                constexpr size_t size_of_uint16 = sizeof(uint16_t);
                uint16_t&   size = *(uint16_t*)(packet->data + offset);
                uint8_t*    data = &packet->data[size_of_uint16 + offset];
                offset += size_of_uint16 + size;
                UpdateData(packet->id, data, size);
            }
            client->seq = packet->seq;
            client->ack = packet->ack;
        }


        int send_size = 0; //mudp.Sendmmsg(send_mmhs, MNSU);

        if (send_size <= 0)
        {
            return;
        }

        UpdateAfterSendmmsg(send_size);
    }

private:

    void UpdateClients() noexcept
    {
        for (size_t i = 0; i < client_pool.Size(); ++i)
        {
            if (!client_pool.IsUsed(i))
            {
                continue;
            }

            auto& client = client_pool[i];
            
            client->timer_for_using.Update(frame);
            client->timer_for_send.Update(frame);

            if (client->timer_for_using.IsExpired())
            {
                client.Release();
                continue;
            }

            if (client->timer_for_send.IsExpired())
            {
                for (uint16_t ack_i = client->ack; ack_i != client->send_seq + 1; ++ack_i)
                {
                    client->datas.pop_front();
                }

                for (int i = client->datas.size(); i >= 0; --i)
                {
                    
                }
            }           
        }
    }

    void SetupRecvmmsg() noexcept
    {
        for (size_t i = 0; i < MNRU; ++i)
        {
            recv_addres_indexs[i] = addres_pool.New();
            recv_packet_indexs[i] = packet_pool.New();
        }

        for (size_t i = 0; i < MNRU; ++i)
        {
            auto& msg_hdr       = recv_mmhs[i].msg_hdr;
            msg_hdr.msg_name    = &addres_pool[recv_addres_indexs[i]];
            msg_hdr.msg_namelen = sizeof(sockaddr_in);
            auto& iovecs        = recv_iovecss[i];
            msg_hdr.msg_iov     = iovecs;
            msg_hdr.msg_iovlen  = TURN_ELEMENT_SIZE;
            auto& packet        = packet_pool[recv_packet_indexs[i]];
            packet->SetToIovecs(iovecs, TURN_ELEMENT_SIZE);
        }
    }

    void UpdateData(uint16_t id, uint8_t* data, uint16_t size) noexcept
    {

    }

    void UpdateAfterSendmmsg(int send_size) noexcept
    {
    }

};

int TURN_TEST()
{
    Turn<4888> turn;
    while (true)
        turn.Update();
    return 0;
}

#endif

