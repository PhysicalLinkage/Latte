
#ifndef FRAME_HPP
#define FRAME_HPP

#include <time.h>

struct Frame_
{
private:
    size_t count;
    timespec tp;
    long delta;
public:
    explicit Frame_() noexcept;
    void Update() noexcept;
    const size_t& Count() const noexcept;
    const long& Time() const noexcept;
    const long& NanoTime() const noexcept;
    const long& DeltaTime() const  noexcept;
};

extern Frame_ Frame;

#endif

