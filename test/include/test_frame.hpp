
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include <frame.hpp>

int test_frame()
{
    printf("Time        : %lus\n", Frame.Time());
    printf("Nano time   : %luns\n", Frame.NanoTime());
    printf("\n");

    auto heavy = []
    {
        size_t sum = 0;
        for (size_t i = 0; i < 1e8; ++i)
        {
            sum += i;
        }
    };

    for (size_t i = 0; i < 8; ++i)
    {
        Frame.Update();
        printf("Frame count : %lu\n", Frame.Count());
        printf("Delta time  : %ldns\n", Frame.DeltaTime());

        heavy();
        printf("Heavy\n");
        printf("\n");
    }

    return 0;
}

