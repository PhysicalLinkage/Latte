
#include <frame.hpp>

Frame_::Frame_() noexcept
    : count {0}
    , tp {}
    , delta {0}
{
    clock_gettime(CLOCK_REALTIME_COARSE, &tp);
}

void Frame_::Update() noexcept
{
    ++count;

    long prev_nsec = tp.tv_nsec;
    clock_gettime(CLOCK_REALTIME_COARSE, &tp);
    delta = tp.tv_nsec - prev_nsec;
    delta += (delta < 0) ? 1e9 : 0;
}

const size_t& Frame_::Count() const noexcept
{
    return count;
}

const long& Frame_::Time() const noexcept
{
    return tp.tv_sec;
}

const long& Frame_::NanoTime() const noexcept
{
    return tp.tv_nsec;
}

const long& Frame_::DeltaTime() const  noexcept
{
    return delta;
}

Frame_ Frame{};

