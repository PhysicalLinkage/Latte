
#ifndef FRAME_HPP
#define FRMAE_HPP

#include <stdint.h>
#include <time.h>
#include <vector>

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

    const auto& Count() const noexcept
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

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

static double CountDate(size_t max, double interval)
{
    return max * interval / 360 / 24 / 360;
}

static int FRAME_TEST()
{
    double s = 0.02;
    for (double i = 0.01; i <= s; i += 0.001)
    {
        printf("%fs : %f\n", i, CountDate(UINT32_MAX, i));
    }
    return 0;
}

#endif

