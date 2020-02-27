
#ifndef FRAME_HPP
#define FRAME_HPP

#include <time.h>

struct Frame
{
private:
    size_t count;
    timespec tp;
    long delta;
public:
    explicit Frame() noexcept
        : count {0}
        , tp {}
        , delta {0}
    {
        clock_gettime(CLOCK_REALTIME_COARSE, &tp);
    }
    
    void Update() noexcept
    {
        ++count;

        long prev_nsec = tp.tv_nsec;
        clock_gettime(CLOCK_REALTIME_COARSE, &tp);
        delta = tp.tv_nsec - prev_nsec;
        delta += (delta < 0) ? 1e9 : 0;
    }

    const size_t& Count() const noexcept
    {
        return count;
    }
    const long& Time() const noexcept
    {
        return tp.tv_sec;
    }
    const long& NanoTime() const noexcept
    {
        return tp.tv_nsec;
    }
    const long& DeltaTime() const  noexcept
    {
        return delta;
    }
};

#endif

