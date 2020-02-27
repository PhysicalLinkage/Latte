
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include <frame.hpp>

int test_frame()
{
    Frame frame;

    printf("Time        : %lus\n", frame.Time());
    printf("Nano time   : %luns\n", frame.NanoTime());
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
        frame.Update();
        printf("Frame count : %lu\n", frame.Count());
        printf("Delta time  : %ldns\n", frame.DeltaTime());

        heavy();
        printf("Heavy\n");
        printf("\n");
    }

    return 0;
}

