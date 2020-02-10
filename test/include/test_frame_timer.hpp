
#include <stdlib.h>
#include <stdio.h>

#include <frame_timer.hpp>

int test_frame_timer()
{
    FrameTimer timer;

    timer.Setup(1e9);

    srand(48);

    size_t s = 0;

    while (s <= 8)
    {
        Frame.Update();
        timer.Update();

        if (timer.IsExpired())
        {
            timer.Reset();
            printf("%lus\n", s++);
        }
    }

    return 0;
}

