
#ifndef UTIL_HPP
#define UTIL_HPP

#include <stddef.h>

constexpr size_t Pow2(size_t x, size_t memo = 1)
{
    return (x == 0) ? memo : Pow2(x - 1, memo << 1);
}

constexpr size_t Log2(size_t x, size_t memo = 0)
{
    return (x == 1) ? memo : Log2(x >> 1, memo + 1);
}

#endif

