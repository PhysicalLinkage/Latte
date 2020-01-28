
#ifndef TURN_PACKET_HPP
#define TURN_PACKET_HPP

#include <cmac.hpp>
#include <sys/socket.h>
#include <string>

constexpr size_t TURN_MAC_SIZE  = CMAC_MAC_SIZE;
constexpr size_t TURN_ID_SIZE   = sizeof(uint16_t);
constexpr size_t TURN_SEQ_SIZE  = sizeof(uint32_t);
constexpr size_t TURN_ACK_SIZE  = sizeof(uint32_t);
constexpr size_t TURN_X_SIZE    = sizeof(float);
constexpr size_t TURN_Y_SIZE    = sizeof(float);
constexpr size_t TURN_Z_SIZE    = sizeof(float);
constexpr size_t TURN_HEADER_SIZE = 
    TURN_MAC_SIZE   +
    TURN_ID_SIZE    +
    TURN_SEQ_SIZE   +
    TURN_ACK_SIZE   +
    TURN_X_SIZE     +
    TURN_Y_SIZE     +
    TURN_Z_SIZE;
constexpr size_t TURN_ELEMENT_SIZE = 8;

template<size_t MMU>
struct TurnPacket
{
    // rudps
    uint8_t mac[CMAC_MAC_SIZE];
    uint16_t id;
    uint32_t seq;
    uint32_t ack;

    // position
    float x;
    float y;
    float z;

    // data
    uint8_t data[MMU - TURN_HEADER_SIZE];

    void SetToIovecs(iovec* iovecs, size_t size) noexcept
    {
        // rudp header
        iovecs[0].iov_base  = mac;
        iovecs[0].iov_len   = sizeof(mac);
        iovecs[1].iov_base  = &id;
        iovecs[1].iov_len   = sizeof(id);
        iovecs[2].iov_base  = &seq;
        iovecs[2].iov_len   = sizeof(seq);
        iovecs[3].iov_base  = &ack;
        iovecs[3].iov_len   = sizeof(ack);

        // position header
        iovecs[4].iov_base  = &x;
        iovecs[4].iov_len   = sizeof(x);
        iovecs[5].iov_base  = &y;
        iovecs[5].iov_len   = sizeof(y);
        iovecs[6].iov_base  = &z;
        iovecs[6].iov_len   = sizeof(z);

        // data
        iovecs[7].iov_base  = data;
        iovecs[7].iov_len   = size;
    }
private:
    bool TryGetMac(CMAC& cmac, uint8_t* mac, uint8_t* key, uint32_t size) noexcept
    {
        return (cmac.Init(key))

            // rudp header
            && (cmac.Update(id))
            && (cmac.Update(seq))
            && (cmac.Update(ack))

            // position header
            && (cmac.Update(x))
            && (cmac.Update(y))
            && (cmac.Update(z))

            // data
            && (cmac.Update(data, size))

            && (cmac.Final(mac));
    }
public:
    bool TryCheckMac(CMAC& cmac, uint8_t* key, size_t size) noexcept
    {
        uint8_t calculated_mac[TURN_MAC_SIZE];
        return (TryGetMac(cmac, calculated_mac, key, size - TURN_HEADER_SIZE))
            && (memcmp(mac, calculated_mac, size) == 0);
    }
    bool TrySetMac(CMAC& cmac, uint8_t* key, size_t size) noexcept
    {
        return TryGetMac(cmac, mac, key, size);
    }

    void DebugLog(FILE* fp) noexcept
    {
        auto fprintf_mac = [&](char* name, uint8_t* mac)
        {
            fprintf(fp, name);
            for (size_t i = 0; i < CMAC_MAC_SIZE; ++i)
                fprintf(fp, "%x", mac[i]);
            fprintf(fp, "\n");
        };

        auto fprintf_dat = [&](char* name, uint8_t* dat)
        {
            fprintf(fp, name);
            for (size_t i = 0; i < sizeof(dat); ++i)
                fprintf(fp, "%x", dat[i]);
            fprintf(fp, "\n");
        };
        
        fprintf_mac("mac    : \n", mac);
        fprintf(fp, "id     : %u\n", id);
        fprintf(fp, "seq    : %u\n", seq);
        fprintf(fp, "ack    : %u\n", ack);
        fprintf(fp, "x      : %f\n", x);
        fprintf(fp, "y      : %f\n", y);
        fprintf(fp, "z      : %f\n", z);
        fprintf_dat("data   : \n", data);
    }

};



static int TURN_PACKET_TEST()
{
    return 0;
}

#endif
