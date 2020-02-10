
#ifndef DHL_HPP
#define DHL_HPP

#include <openssl/rand.h>
#include <dh_pg.hpp>

#define DHL_KEY_SIZE 256

static const char rand_seed[] = "t48kfb,ep*oa<ue+yt4,em.z@nz*:b{9ha-gfqp_ee1}bv";

void setup_rand_seed() noexcept
{
    RAND_seed(rand_seed, sizeof(rand_seed));
}

class DHL
{
private:
    DH* dh;
public:
    uint8_t public_key[DHL_KEY_SIZE];
    
    explicit DHL() noexcept
        : dh {get_dh2048()}
    {
        if ((dh == NULL) || (!DH_generate_key(dh)))
        {
            fprintf(stderr, "dh == NULL or !DH_generate_key(dh)");
            exit(1);
        }

        BN_bn2bin(dh->pub_key, public_key);
    }

    void ComputeKey(uint8_t* key, uint8_t* public_key) noexcept
    {
        DH_compute_key(key, BN_bin2bn(public_key, DHL_KEY_SIZE, NULL), dh);
    }

    ~DHL() noexcept
    {
        DH_free(dh);
    }
};

int DHL_TEST()
{
    setup_rand_seed();

    DHL alice;
    uint8_t alice_public_key[DHL_KEY_SIZE];
    uint8_t alice_common_key[DHL_KEY_SIZE];

    DHL bob;
    uint8_t bob_public_key[DHL_KEY_SIZE];
    uint8_t bob_common_key[DHL_KEY_SIZE];

    alice.ComputeKey(alice_common_key, bob.public_key);
    bob.ComputeKey(bob_common_key, alice.public_key);

    auto f = [&](const char* name, uint8_t* data)
    {
        printf("%s : ", name);
        for (size_t i = 0; i < DHL_KEY_SIZE; ++i)
        {
            printf("%x", data[i]);
        }
        printf("\n");
    };

    f("alice public key", alice_public_key);
    f("bob   public key", bob_public_key);
    f("alice", alice_common_key);
    f("bob  ", bob_common_key);

    return 0;
}

#endif

