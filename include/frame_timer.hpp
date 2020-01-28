
#ifndef FRAME_TIMER_HPP
#define FRAME_TIMER_HPP

#include <frame.hpp>

class FrameTimer
{
private:
    long interval;
    long timer;

public:

    void Setup(long nano) noexcept
    {
        timer = interval = (nano <= 0) ? 0 : nano;
    }

    void Update(const Frame& frame) noexcept
    {
        timer -= frame.DeltaTime();
    }

    void Reset() noexcept
    {
        timer = interval;
    }

    bool IsExpired() const noexcept
    {
        return (timer <= 0);
    }
};

static int FRAME_TIMER_TEST()
{
    Frame frame;
    FrameTimer timer;
    FrameTimer rand_timer;

    timer.Setup(3e9);
    rand_timer.Setup(100);
    
    srand(48);

    while (true)
    {
        frame.Update();
        timer.Update(frame);
        rand_timer.Update(frame);

        auto virtual_send = [&]
        {
            printf("send in %lds\n", frame.Time());
            timer.Reset();
        };

        if (rand_timer.IsExpired())
        {
            if (rand() % 1000 == 0)
            {
                printf("random : ");
                virtual_send();
            }
            rand_timer.Reset();
        }

        if (timer.IsExpired())
        {
            printf("normal : ");
            virtual_send();
        }
    }

    return 0;
}

#endif

