
#ifndef SOURCE_HPP
#define SOURCE_HPP

#include <stddef.h>

template<class T>
class Reference
{
private:
    T& x;
    size_t& count;
public:
    explicit Reference(T& x_, size_t& count_) noexcept
        : x     {x_}
        , count {++count_}
    {}
    Reference(const Reference&) = delete;
    Reference& operator=(const Reference&) = delete;
    T& Data() { return x; }
    void Release() noexcept { count = (count == 0) ? 0 : count - 1; }
};

template<class T, class... Args>
class Source
{
private:
    T x;
    size_t count;
public:
    explicit Source(Args... args) noexcept
        : x     {args...}
        , count {0}
    {}
    Source(const Source&) = delete;
    Source& operator=(const Source&) = delete;
    bool IsUsed() noexcept { return count != 0; }
    Reference<T> Share() noexcept { return Reference<T>(x, count); }
};


#include <stdint.h>
#include <iostream>

struct info
{
    uint16_t id;
    uint16_t pw;

    info(uint16_t id_, uint16_t pw_)
        : id {id_}
        , pw {pw_}
    {}
};

int SOURCE_TEST()
{
    Source<info, uint16_t, uint16_t> a(535, 48);
    auto ref1 = a.Share();
    auto ref2 = a.Share();
    std::cout << ref1.Data().id << "\n";
    std::cout << ref1.Data().pw << "\n";
    std::cout << a.IsUsed() << "\n";
    ref1.Release();
    std::cout << a.IsUsed() << "\n";
    ref2.Release();
    std::cout << a.IsUsed() << "\n";
    return 0;
}

#endif

