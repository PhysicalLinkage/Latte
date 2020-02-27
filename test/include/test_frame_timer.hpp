
#include <stdlib.h>
#include <stdio.h>

#include <frame_timer.hpp>

int test_frame_timer()
{
    Frame frame;
    FrameTimer timer;

    timer.Setup(1e9);

    srand(48);

    size_t s = 0;

    while (s <= 8)
    {
        frame.Update();
        timer.Update(frame);

        if (timer.IsExpired())
        {
            timer.Reset();
            printf("%lus\n", s++);
        }
    }

    return 0;
}

