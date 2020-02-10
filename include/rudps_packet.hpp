
#ifndef RUDPS_PACKET_HPP
#define RUDPS_PACKET_HPP

#include <cmac.hpp>
#include <sys/socket.h>
#include <string>

constexpr size_t RUDPS_MAC_SIZE  = CMAC_MAC_SIZE;
constexpr size_t RUDPS_CTR_SIZE  = sizeof(uint32_t);
constexpr size_t RUDPS_ID_SIZE   = sizeof(uint16_t);
constexpr size_t RUDPS_SEQ_SIZE  = sizeof(uint16_t);
constexpr size_t RUDPS_ACK_SIZE  = sizeof(uint16_t);
constexpr size_t RUDPS_X_SIZE    = sizeof(float);
constexpr size_t RUDPS_Y_SIZE    = sizeof(float);
constexpr size_t RUDPS_Z_SIZE    = sizeof(float);
constexpr size_t RUDPS_HEADER_SIZE = 
    RUDPS_MAC_SIZE   +
    RUDPS_CTR_SIZE   +
    RUDPS_ID_SIZE    +
    RUDPS_SEQ_SIZE   +
    RUDPS_ACK_SIZE   +
    RUDPS_X_SIZE     +
    RUDPS_Y_SIZE     +
    RUDPS_Z_SIZE;
constexpr size_t RUDPS_ELEMENT_SIZE = 6;

struct RUDPSPacket
{
    uint8_t* mac;
    uint32_t* counter;
    uint16_t* id;
    uint16_t* seq;
    uint16_t* ack;
    uint8_t* data;

private:
    bool TryGetMac(CMAC& cmac, uint8_t* mac, uint8_t* key, uint32_t size) noexcept
    {
        return (cmac.Init(key))
            && (cmac.Update(*counter))
            && (cmac.Update(*id))
            && (cmac.Update(*seq))
            && (cmac.Update(*ack))
            && (cmac.Update(data, size))
            && (cmac.Final(mac));
    }
public:
    bool TryCheckMac(CMAC& cmac, uint8_t* key, size_t size) noexcept
    {
        uint8_t calculated_mac[RUDPS_MAC_SIZE];
        return (TryGetMac(cmac, calculated_mac, key, size - RUDPS_HEADER_SIZE))
            && (memcmp(mac, calculated_mac, CMAC_MAC_SIZE) == 0);
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
        
        fprintf_mac("mac    : \n", mac);
        fprintf(fp, "counter: %u\n", *counter);
        fprintf(fp, "id     : %u\n", *id);
        fprintf(fp, "seq    : %u\n", *seq);
        fprintf(fp, "ack    : %u\n", *ack);
        fprintf(fp, "data   : %s\n", data);
    }

};

static int RUDPS_PACKET_TEST()
{
    return 0;
}

#endif
