#ifndef SERVERLESS_HPP
#define SERVERLESS_HPP

#include <old_mudp.hpp>
#include <random>
#include <ctime>

namespace Latte
{

namespace Linux
{

namespace Network
{

struct User
{
    bool        is_used;
    uint16_t    passwords;
    IPEndpoint  endpoint;
    time_t      timestamp;

    explicit User() noexcept
        : is_used   {false}
        , passwords {0}
        , endpoint  {}
        , timestamp {0}
    {
    }
};

class UserManager
{
public:
    constexpr static size_t     Size        = UINT16_MAX + 1;
    constexpr static uint16_t   RoomSize    = 4;
    constexpr static uint16_t   RoomNumber  = 128;
    constexpr static uint16_t   WorldSize   = RoomNumber * RoomSize;
    constexpr static uint16_t   WorldNumber = Size / WorldSize;

private:
    struct User     users[Size];
    std::mt19937_64 rand;

    uint16_t Index(uint16_t world_i, uint16_t room_i, uint16_t number_i) const noexcept
    {
        return world_i * WorldSize + room_i * RoomSize + number_i;
    }

    struct User& User(uint16_t world_i, uint16_t room_i, uint16_t number_i) noexcept
    {
        return users[Index(world_i, room_i, number_i)];
    }

    const struct User& User(uint16_t world_i, uint16_t room_i, uint16_t number_i) const noexcept
    {
        return users[Index(world_i, room_i, number_i)];
    }

    bool IsRangeRoomSize(uint16_t number_i) const noexcept
    {
        return number_i < RoomSize;
    }

    bool IsRangeRoomNumber(uint16_t room_i) const noexcept
    {
        return room_i < RoomNumber;
    }

    bool IsRangeWorldNumber(uint16_t world_i) const noexcept
    {
        return world_i < WorldNumber;
    }

public:
    explicit UserManager(uint64_t seed) noexcept
        : rand(seed)
    {
    }

    void add(
        uint16_t            world_i, 
        uint16_t            room_i, 
        const IPEndpoint&   endpoint, 
        uint16_t&           number,
        bool&               is_success, 
        uint16_t&           id,
        uint16_t&           pw) noexcept
    {
        is_success = false;

        if (!IsRangeWorldNumber(world_i)) return;
        if (!IsRangeRoomNumber(room_i)) return;

        for (uint16_t number_i = 0; number_i < RoomSize; ++number_i)
        {
            auto& new_user = User(world_i, room_i, number_i);

            if (new_user.is_used) continue;
            
            for (auto& user : users)
            {
                if (user.endpoint == endpoint) user.is_used = false;
            }
            
            number = number_i;
            is_success = new_user.is_used = true;
            id = Index(world_i, room_i, number_i);
            pw = new_user.passwords =( rand() % UINT16_MAX) + 1;
            new_user.endpoint = endpoint;
            new_user.timestamp = time(nullptr);
            return;
        }
    }

    void remove(uint16_t id, uint16_t pw)
    {
        if (users[id].passwords == pw) users[id].is_used = false;
    }

    template<class T>
    void TryGetEndpoint(uint16_t id, uint16_t pw, uint16_t de, const T& on) const noexcept
    {
        if (!users[id].is_used) return;
        if (users[id].passwords != pw) return;
        if (id == de) return;
        if (!users[de].is_used) return;

        on(users[de].endpoint);
    }

    template<class T>
    void TryGetEndpoint(uint16_t id, uint16_t pw, uint16_t wi, uint16_t ri, const T& on) const noexcept
    {
        if (!users[id].is_used) return;
        if (users[id].passwords != pw) return;

        for (int ni = 0; ni < RoomSize; ++ni)
        {
            if (id == Index(wi, ri, ni)) continue;
            
            auto& de_user = User(wi, ri, ni);

            if (!de_user.is_used) continue;

            on(de_user.endpoint);
        }
    }

    uint16_t TotalRoomUseds(uint16_t world_i, uint16_t room_i) const noexcept
    {
        uint16_t total = 0;
        for (uint16_t number_i = 0; number_i < RoomSize; ++number_i)
        {
            total += (User(world_i, room_i, number_i).is_used) ? 1 : 0;
        }
        return total;
    }

    uint16_t TotalWorldUseds(uint16_t world_i) const noexcept
    {
        uint16_t total = 0;
        for (uint16_t room_i = 0; room_i < WorldSize; ++room_i)
        {
            total += TotalRoomUseds(world_i, room_i);
        }
        return total;
    }
};

}

}

}


int SERVERLESS_TEST()
{

    enum Protocol
    {
        WORLD_TYPE      = 1,
        ROOM_TYPE       = 2,
        LOGIN_TYPE      = 3,
        LOGOUT_TYPE     = 4,
        TURN_TYPE       = 5,
        ROOM_TURN_TYPE  = 6
    };

    using namespace Latte::Linux::Network;
    using namespace Latte;

    MUDP<53548> mudp;

    UserManager manager(488);

    while (true)
    {
        const int received_number = mudp.MultipleReceive();
        if (received_number > 0)
        {
            printf("-------------------\n");
            printf("recieved : %d\n", received_number);

            for (int i = 0; i < received_number; ++i)
            {
                const auto& rp = mudp.ReceivedPacket(i);

                if (WORLD_TYPE == (Protocol)rp.Message()[0])
	            {
                    auto& sp = mudp.SendedPacket();
                    sp.Endpoint() = rp.Endpoint();

                    *(char*)    (sp.Message())                                      = rp.Message()[0];
                    *(uint16_t*)(sp.Message() + sizeof(char))                       = manager.WorldSize;
                    *(uint16_t*)(sp.Message() + sizeof(char) + sizeof(uint16_t))    = manager.WorldNumber;
		            for (uint16_t index = 0; index < manager.WorldNumber; ++index)
		            {
			            *(uint16_t*)(sp.Message() + sizeof(char) + sizeof(uint16_t) * (2 + index)) = manager.TotalWorldUseds(index);
		            }
		            constexpr size_t size = sizeof(char) + sizeof(uint16_t) * (2 + manager.WorldNumber);
		            sp.Size() = size;
	            }
	            else if (ROOM_TYPE == (Protocol)rp.Message()[0])
	            {
                    auto& sp = mudp.SendedPacket();
                    sp.Endpoint() = rp.Endpoint();

		            const uint16_t& world_index = *(uint16_t*)(rp.Message() + sizeof(char));
                    *(char*)    (sp.Message())                                          = rp.Message()[0];
                    *(uint16_t*)(sp.Message() + sizeof(char))                           = world_index;
                    *(uint16_t*)(sp.Message() + sizeof(char) + sizeof(uint16_t))        = manager.RoomSize;
                    *(uint16_t*)(sp.Message() + sizeof(char) + sizeof(uint16_t) * 2)    = manager.RoomNumber;
		            for (uint16_t room_index = 0; room_index < manager.RoomNumber; ++room_index)
		            {
			            *(uint16_t*)(sp.Message() + sizeof(char) + sizeof(uint16_t) * (3 + room_index)) = manager.TotalRoomUseds(world_index, room_index);
		            }
		            constexpr size_t size = sizeof(char) + sizeof(uint16_t) * (3 + manager.RoomNumber);
		            sp.Size() = size;
    	        }
	            else if (LOGIN_TYPE == (Protocol)rp.Message()[0])
	            {
                    auto& sp = mudp.SendedPacket();
                    sp.Endpoint() = rp.Endpoint();
                    memcpy(sp.Message(), rp.Message(), rp.Size());
                    
		            const uint16_t& world_index = *(uint16_t*)(rp.Message() + sizeof(char));
		            const uint16_t& room_index = *(uint16_t*)(rp.Message() + sizeof(char) + sizeof(uint16_t));

		            manager.add(world_index, room_index, sp.Endpoint()
                        , *(uint16_t*)(sp.Message() + sizeof(char) + sizeof(uint16_t) * 2)
			            , *(bool*)(sp.Message() + sizeof(char) + sizeof(uint16_t) * 3)
			            , *(uint16_t*)(sp.Message() + sizeof(char) * 2 + sizeof(uint16_t) * 3)
			            , *(uint16_t*)(sp.Message() + sizeof(char) * 2 + sizeof(uint16_t) * 4));
	    	        constexpr size_t size = sizeof(char) * 2 + sizeof(uint16_t) * 5;
	    	        sp.Size() = size;
	            }
	            else if (LOGOUT_TYPE == (Protocol)rp.Message()[0])
	            {
		            uint16_t& id = *(uint16_t*)(rp.Message() + sizeof(char));
		            uint16_t& passwords = *(uint16_t*)(rp.Message() + sizeof(char) + sizeof(uint16_t));
		            manager.remove(id, passwords);
	            }
	            else if (TURN_TYPE == (Protocol)rp.Message()[0])
	            {
                    auto& sp = mudp.SendedPacket();
                    sp.Size() = rp.Size();
                    memcpy(sp.Message(), rp.Message(), rp.Size());

		            const uint16_t& source = *(uint16_t*)(sp.Message() + sizeof(char));
		            uint16_t& passwords = *(uint16_t*)(sp.Message() + sizeof(char) + sizeof(uint16_t));
		            const uint16_t& destination = *(uint16_t*)(sp.Message() + sizeof(char) + sizeof(uint16_t) * 2);
		            manager.TryGetEndpoint(source, passwords, destination, [&sp, &passwords](const IPEndpoint& endpoint)
			        {
				        passwords = 0;
				        sp.Endpoint() = endpoint;
			        });
	            }
	            else if (ROOM_TURN_TYPE == (Protocol)rp.Message()[0])
	            {

		            const uint16_t& source = *(uint16_t*)(rp.Message() + sizeof(char));
		            const uint16_t& passwords = *(uint16_t*)(rp.Message() + sizeof(char) + sizeof(uint16_t));
		            const uint16_t& world_index = *(uint16_t*)(rp.Message() + sizeof(char) + sizeof(uint16_t) * 2);
		            const uint16_t& room_index = *(uint16_t*)(rp.Message() + sizeof(char) + sizeof(uint16_t) * 3);
		            manager.TryGetEndpoint(source, passwords, world_index, room_index, [&](const IPEndpoint& endpoint)
			        {
                        auto& sp = mudp.SendedPacket();
                        sp.Endpoint() = endpoint;
                        sp.Size() = rp.Size();
                        printf("sp.Size() %lu", sp.Size());
                        memcpy(sp.Message(), rp.Message(), rp.Size());
				        *(uint16_t*)(sp.Message() + sizeof(char) + sizeof(uint16_t)) = 0;
			        });
	            }
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



