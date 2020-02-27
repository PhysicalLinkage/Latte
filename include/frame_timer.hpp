
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

    void Update(Frame& frame) noexcept
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

#endif

