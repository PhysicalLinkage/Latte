
#ifndef CMAC_HPP
#define CMAC_HPP

#include <openssl/cmac.h>

#define CMAC_KEY_SIZE 16
#define CMAC_MAC_SIZE 16

class CMAC
{
private:
    CMAC_CTX* cmac_ctx;
    
public:

    explicit CMAC() noexcept
        : cmac_ctx {CMAC_CTX_new()}
    {}

    bool Init(uint8_t* key) noexcept
    {
        return CMAC_Init(cmac_ctx, key, CMAC_KEY_SIZE, EVP_aes_128_cbc(), NULL) == 1;
    }

    bool Update(void* message, size_t size) noexcept
    {
        return CMAC_Update(cmac_ctx, message, size) == 1;
    }

    template<class T>
    bool Update(T& message) noexcept
    {
        return Update(&message, sizeof(T));
    }


    bool Final(uint8_t* mac) noexcept
    {
        size_t size;
        return CMAC_Final(cmac_ctx, mac, &size) == 1;
    }

    ~CMAC() noexcept
    {
        CMAC_CTX_free(cmac_ctx);
    }
};

#include <string.h>

int CMAC_TEST()
{
    CMAC cmac;
    uint8_t key[CMAC_KEY_SIZE] = 
    {
        0x2b,0x7e,0x15,0x16, 0x28,0xae,0xd2,0xa6,
        0xab,0xf7,0x15,0x88, 0x09,0xcf,0x4f,0x3c,
    };
    uint8_t message[] = "AYAYA";
    uint8_t mac[CMAC_MAC_SIZE];


    if (!cmac.Init(key))
        return 1;
    if (!cmac.Update(message, sizeof(message)))
        return 1;
    if (!cmac.Final(mac))
        return 1;

    printf("mac = ");
    for (size_t i = 0; i < CMAC_MAC_SIZE; ++i)
        printf("%x", mac[i]);
    printf("\n");

    return 0;
}

#endif

