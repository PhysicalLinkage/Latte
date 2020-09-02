
#ifndef GACHI_LAND_SERVER_HPP
#define GACHI_LAND_SERVER_HPP

#include <server_rudps.hpp>
#include <util.hpp>

#define GL_TYPE_WORLD       0
#define GL_TYPE_ROOM        1
#define GL_TYPE_LOGIN       2
#define GL_TYPE_LOGOUT      3
#define GL_TYPE_TURN        4
#define GL_TYPE_ROOM_TURN   5
#define GL_TYPE_SIZE        6

constexpr size_t GL_TYPE_POW2_SIZE = Pow2(Log2(GL_TYPE_SIZE-1)+1);

class GachiLandServer : public ServerRUDPS
{
    constexpr static size_t    SIZE         = UINT16_MAX + 1;
    constexpr static uint16_t ROOM_SIZE     = 4;
    constexpr static uint16_t ROOM_NUMBER   = 40;
    constexpr static uint16_t WORLD_SIZE    = ROOM_NUMBER * ROOM_SIZE;
    constexpr static uint16_t WORLD_NUMBER  = 40;
private:
    void (GachiLandServer::*on_recvs[GL_TYPE_POW2_SIZE])
        (SRUDPS& rudps,
         iovec& iov,
         std::shared_ptr<Message> message);
public:
    explicit GachiLandServer(uint16_t port) noexcept
        : ServerRUDPS {port}
        , on_recvs {}
        , users {0}
    {
        on_recvs[GL_TYPE_WORLD]     = &GachiLandServer::OnRecvWorld;
        on_recvs[GL_TYPE_ROOM]      = &GachiLandServer::OnRecvRoom;
        on_recvs[GL_TYPE_LOGIN]     = &GachiLandServer::OnRecvLogin;
        on_recvs[GL_TYPE_LOGOUT]    = &GachiLandServer::OnRecvLogout;
        on_recvs[GL_TYPE_TURN]      = &GachiLandServer::OnRecvTurn;
        on_recvs[GL_TYPE_ROOM_TURN] = &GachiLandServer::OnRecvRoomTurn;

        for (size_t i = GL_TYPE_SIZE; i < GL_TYPE_POW2_SIZE; ++i)
        {
            on_recvs[i] = &GachiLandServer::OnRecvEmpty;
        }
    }

    struct User
    {
        bool is_used;
        uint16_t id;
    };

    User users[SIZE];

    struct LogoutPacket : public RUDPS::SendPacket
    {
        uint8_t type;
        uint16_t id;
        uint16_t pw;
        uint16_t world;
        uint16_t room;
    };

    void Update(Frame& frame) noexcept
    {
        RecvUpdate();
        for (size_t i = 0; i < rudpss.size(); ++i)
        {
            if (!rudpss[i]->is_used)
            {
                continue;
            }

            uint16_t login_id = rudpss[i]->login_id;
        
            if (!rudpss[i]->Update(frame))
            {
                auto world_i = WorldIndex(login_id);
                auto room_i = RoomIndex(login_id);

                if (!is_range_id(login_id))
                {
                    continue;
                }

                if (world_i >= WORLD_NUMBER)
                {
                    continue;
                }

                if (room_i >= ROOM_NUMBER)
                {
                    continue;
                }
                
                for (uint16_t number_i = 0; number_i < ROOM_SIZE; ++number_i)
                {
                    auto destination_i = Index(world_i, room_i, number_i);

                    if (login_id == destination_i)
                    {
                        continue;
                    }

                    auto& destination = users[destination_i];

                    if (!destination.is_used)
                    {
                       continue;
                    }

                    auto packet = std::make_unique<LogoutPacket>();
                    packet->type = GL_TYPE_ROOM_TURN;
                    packet->id = login_id;
                    packet->pw = login_id;
                    packet->world = world_i;
                    packet->room = room_i;
                    static char logout_type[] = u8"logout";
                    packet->iovs.resize(6);
                    uint8_t i = 0;
                    packet->iovs[i].iov_base = &packet->type;
                    packet->iovs[i++].iov_len = sizeof(packet->type);
                    packet->iovs[i].iov_base = &packet->id;
                    packet->iovs[i++].iov_len = sizeof(packet->id);
                    packet->iovs[i].iov_base = &packet->pw;
                    packet->iovs[i++].iov_len = sizeof(packet->pw);
                    packet->iovs[i].iov_base = &packet->world;
                    packet->iovs[i++].iov_len = sizeof(packet->world);
                    packet->iovs[i].iov_base = &packet->room;
                    packet->iovs[i++].iov_len = sizeof(packet->room);
                    packet->iovs[i].iov_base = logout_type;
                    packet->iovs[i++].iov_len = sizeof(logout_type);
                    rudpss[destination.id]->Send(std::move(packet));
                }
                uint32_t host = rudpss[i]->address->sin_addr.s_addr;
                uint16_t port = rudpss[i]->address->sin_port;
                printf("logout: %u, %u\n", host, port);
                contactss[host].erase(port);
                users[login_id].is_used = false;
                rudpss[i]->is_used = false;
            }
            else
            {
                rudpss[i]->SendUpdate(frame, cmac);
            }
        }
        SendUpdate();
    }

private:

    bool is_range_id(uint16_t id) noexcept
    {
        return id < SIZE;
    }

    uint16_t Index(uint16_t world_i, uint16_t room_i, uint16_t number_i) noexcept
    {
        return world_i * WORLD_SIZE + room_i * ROOM_SIZE + number_i;
    }

    uint16_t WorldIndex(uint16_t id) noexcept
    {
        return id % SIZE / WORLD_SIZE;
    }

    uint16_t RoomIndex(uint16_t id) noexcept
    {
        return id % WORLD_NUMBER / ROOM_SIZE;
    }

    uint16_t RoomUsedTotal(uint16_t world_i, uint16_t room_i) noexcept
    {
        uint16_t total = 0;
        for (uint16_t number_i = 0; number_i < ROOM_SIZE; ++number_i)
        {
            total += (users[Index(world_i, room_i, number_i)].is_used) ? 1 : 0;
        }
        return total;
    }

    uint16_t WorldUsedTotal(uint16_t world_i) noexcept
    {
        uint16_t total = 0;
        for (uint16_t room_i = 0; room_i < ROOM_NUMBER; ++room_i)
        {
            total += RoomUsedTotal(world_i, room_i);
        }
        return total;
    }

    void OnRecv(
        SRUDPS& rudps,
        iovec& iov,
        std::shared_ptr<Message> message) noexcept override
    {
        constexpr size_t gl_type_pow2_max = GL_TYPE_POW2_SIZE - 1;
        uint8_t type = *(uint8_t*)iov.iov_base & gl_type_pow2_max;
        (this->*on_recvs[type])(rudps, iov, std::move(message));
    }

    struct World
    {
        uint8_t     type;
        uint16_t    size;
        uint16_t    number;
        uint16_t    used_totals[WORLD_NUMBER];

        size_t Size() noexcept
        {
            return  sizeof(type) 
                +   sizeof(size) 
                +   sizeof(number)
                +   sizeof(used_totals);
        }
    };

    struct WorldPacket : public RUDPS::SendPacket
    {
        World world;
        std::shared_ptr<Message> message;
    };

    void OnRecvWorld(
        SRUDPS& rudps,
        iovec& iov,
        std::shared_ptr<Message> message) noexcept
    {
        auto packet = std::make_unique<WorldPacket>();
        packet->world.type      = *(uint8_t*)iov.iov_base;
        packet->world.size      = WORLD_SIZE;
        packet->world.number    = WORLD_NUMBER;
        for (uint16_t i = 0; i < WORLD_NUMBER; ++i)
        {
            packet->world.used_totals[i] = WorldUsedTotal(i);
        }

        packet->iovs.resize(4);
        packet->iovs[0].iov_base = &packet->world.type;
        packet->iovs[0].iov_len = sizeof(packet->world.type);
        packet->iovs[1].iov_base = &packet->world.size;
        packet->iovs[1].iov_len = sizeof(packet->world.size);
        packet->iovs[2].iov_base = &packet->world.number;
        packet->iovs[2].iov_len = sizeof(packet->world.number);
        packet->iovs[3].iov_base = packet->world.used_totals;
        packet->iovs[3].iov_len = sizeof(packet->world.used_totals);
        packet->message = std::move(message);
        rudps.Send(std::move(packet));
    }

    struct Room
    {
        uint16_t size;
        uint16_t number;
        uint16_t used_totals[ROOM_NUMBER];
    };

    struct RoomPacket : public RUDPS::SendPacket
    {
        Room room;
        std::shared_ptr<Message> message;
    };


    void OnRecvRoom(
        SRUDPS& rudps,
        iovec& iov,
        std::shared_ptr<Message> message) noexcept
    {
        auto& world_i = *(uint16_t*)((uint8_t*)iov.iov_base + sizeof(uint8_t));
        auto packet = std::make_unique<RoomPacket>();
        packet->room.size   = ROOM_SIZE;
        packet->room.number = ROOM_NUMBER;
        for (uint16_t room_i = 0; room_i < ROOM_NUMBER; ++room_i)
        {
            packet->room.used_totals[room_i] = RoomUsedTotal(world_i, room_i);
        }

        size_t i = 0;
        packet->iovs.resize(4);
        packet->iovs[i].iov_base    = iov.iov_base;
        packet->iovs[i++].iov_len   = sizeof(uint8_t) + sizeof(uint16_t);
        packet->iovs[i].iov_base    = &packet->room.size;
        packet->iovs[i++].iov_len   = sizeof(packet->room.size);
        packet->iovs[i].iov_base    = &packet->room.number;
        packet->iovs[i++].iov_len   = sizeof(packet->room.number);
        packet->iovs[i].iov_base    = packet->room.used_totals;
        packet->iovs[i++].iov_len   = sizeof(packet->room.used_totals);
        packet->message = std::move(message);
        rudps.Send(std::move(packet));
    }

    struct Login
    {
        uint16_t number_i;
        bool is_success;
        uint16_t id;
    };

    Login Add(
        uint16_t    world_i,
        uint16_t    room_i,
        uint16_t    rid) noexcept
    {
        Login login;
        login.is_success = false;

        if (world_i >= WORLD_NUMBER) return login;
        if (room_i >= ROOM_NUMBER) return login;

        for (uint16_t number_i = 0; number_i < ROOM_SIZE; ++number_i)
        {
            uint16_t login_id = Index(world_i, room_i, number_i);
            auto& user = users[login_id];

            if (user.is_used) continue;

            user.is_used = true;
            user.id = rid;
            auto& rudps = rudpss[rid];
            rudps->is_used = true;
            rudps->login_id = login_id;
            login.number_i = number_i;
            login.is_success = true;
            login.id = login_id;
            return login;
        }

        return login;
    }

     struct LoginPacket : public RUDPS::SendPacket
    {
        Login login;
        std::shared_ptr<Message> message;
    };

    void OnRecvLogin(
        SRUDPS& rudps,
        iovec& iov,
        std::shared_ptr<Message> message) noexcept
    {
        auto& world_i = *(uint16_t*)((uint8_t*)iov.iov_base + sizeof(uint8_t));
        auto& room_i = *(uint16_t*)((uint8_t*)iov.iov_base + sizeof(uint8_t) + sizeof(uint16_t));
        auto packet = std::make_unique<LoginPacket>();
        if (users[rudps.login_id].is_used)
        {
            return;
        }
        packet->login = Add(world_i, room_i, rudps.ID());

        size_t i = 0;
        packet->iovs.resize(5);
        packet->iovs[i].iov_base    = iov.iov_base;
        packet->iovs[i++].iov_len   = sizeof(uint8_t) + sizeof(uint16_t) * 2;
        packet->iovs[i].iov_base    = &packet->login.number_i;
        packet->iovs[i++].iov_len   = sizeof(packet->login.number_i);
        packet->iovs[i].iov_base    = &packet->login.is_success;
        packet->iovs[i++].iov_len   = sizeof(packet->login.is_success);
        packet->iovs[i].iov_base    = &packet->login.id;
        packet->iovs[i++].iov_len   = sizeof(packet->login.id);
        packet->iovs[i].iov_base    = &packet->login.id;
        packet->iovs[i].iov_len     = sizeof(packet->login.id);

        packet->message = std::move(message);
        rudps.Send(std::move(packet));
    }

    void OnRecvLogout(
        SRUDPS& rudps,
        iovec& iov,
        std::shared_ptr<Message> message) noexcept
    {
    }

    struct TurnPacket : public RUDPS::SendPacket
    {
        std::shared_ptr<Message> message;
    };

    void OnRecvTurn(
        SRUDPS& rudps,
        iovec& iov,
        std::shared_ptr<Message> message) noexcept
    {
        auto& login_id = *(uint16_t*)((uint8_t*)iov.iov_base + sizeof(uint8_t));
        auto& destination_id = *(uint16_t*)((uint8_t*)iov.iov_base + sizeof(uint8_t) + sizeof(uint16_t) * 2);

        if (!is_range_id(login_id))
        {
            return;
        }

        if (!is_range_id(destination_id))
        {
            return;
        }

        if (!users[login_id].is_used)
        {
            return;
        }

        auto& destination = users[destination_id];

        if (!destination.is_used)
        {
            return;
        }

        auto packet = std::make_unique<TurnPacket>();
        packet->iovs.resize(1);
        packet->iovs[0].iov_base = iov.iov_base;
        packet->iovs[0].iov_len = iov.iov_len;
        packet->message = std::move(message);
        rudpss[destination.id]->Send(std::move(packet));
    }

    void OnRecvRoomTurn(
        SRUDPS& rudps,
        iovec& iov,
        std::shared_ptr<Message> message) noexcept
    {
        printf("rudps: ");
        for (size_t i = 0; i < iov.iov_len; ++i)
        {
            printf("%c", *((uint8_t*)iov.iov_base + i));
        }
        printf("\n");
        auto& login_id = *(uint16_t*)((uint8_t*)iov.iov_base + sizeof(uint8_t));
        auto& world_i = *(uint16_t*)((uint8_t*)iov.iov_base + sizeof(uint8_t) + sizeof(uint16_t) * 2);
        auto& room_i = *(uint16_t*)((uint8_t*)iov.iov_base + sizeof(uint8_t) + sizeof(uint16_t) * 3);

        if (!is_range_id(login_id))
        {
            return;
        }

        if (world_i >= WORLD_SIZE)
        {
            return;
        }

        if (room_i >= ROOM_NUMBER)
        {
            return;
        }
        
        for (uint16_t number_i = 0; number_i < ROOM_SIZE; ++number_i)
        {
            auto destination_i = Index(world_i, room_i, number_i);

            if (login_id == destination_i)
            {
                continue;
            }

            auto& destination = users[destination_i];

            if (!destination.is_used)
            {
               continue;
            }

            auto packet = std::make_unique<TurnPacket>();
            packet->iovs.resize(1);
            packet->iovs[0].iov_base = iov.iov_base;
            packet->iovs[0].iov_len = iov.iov_len;
            packet->message = std::move(message);
            rudpss[destination.id]->Send(std::move(packet));
        }
    }

    void OnRecvEmpty(
        SRUDPS&,
        iovec&,
        std::shared_ptr<Message>) noexcept
    {
    }

};

int gachi_land_server()
{
    uint32_t port = 7777;
    Frame frame;
    GachiLandServer gls(port);

    printf("start server : port %u\n", port);

    while (true)
    {
        frame.Update();
        gls.Update(frame);
    }

    return 0;
}

#endif

