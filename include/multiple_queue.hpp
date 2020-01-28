
#ifndef MULTIPLE_QUEUE_HPP
#define MULTIPLE_QUEUE_HPP

#include <stddef.h>
#include <string.h>

#include <stdio.h>

namespace Latte
{

constexpr size_t pow2(size_t n, size_t result = 1)
{
    return (n == 0) ? result : pow2(n - 1, result << 1);
}

constexpr size_t log2(size_t x, size_t result = 0)
{
    return (x == 1) ? result : log2(x >> 1, result + 1);
}

template<class T>
class MultipleQueue
{
private:
    T* data;
    size_t begin;
    size_t end;
    size_t capacity;

public:
    constexpr explicit MultipleQueue(size_t capacity_ = 1) noexcept
        : data      {nullptr}
        , begin     {0}
        , end       {0}
        , capacity  {pow2(log2(capacity_ == 0 ? 1 : capacity_))}
    {
        data = new T[capacity];
    }

    ~MultipleQueue() noexcept { delete[] data; }
    
    size_t Size() noexcept 
    { 
        return (begin <= end) ? 
            end - begin : 
            capacity - begin + end;  
    }

    size_t Begin() const noexcept
    {
        return begin;
    }

    size_t End() const noexcept
    {
        return end;
    }

    void AddIndex(size_t& i) noexcept
    {
        i = (i + 1) & (capacity - 1);
    }

    T& Data(size_t i) noexcept
    {
        return data[i];
    }

    template<class F>
    void ForFromBegin(size_t offset, const F& f) noexcept
    {
        size_t point = (begin + offset) & (capacity - 1);
        for (size_t i = begin; i != point; i = (i + 1) & (capacity - 1))
        {
            f(data[i]);
        }
    }

    template<class F>
    void ForToEnd(size_t offset, const F& f) noexcept
    {
        size_t point = (begin + offset) & (capacity - 1);
        for (size_t i = point; i != end; i = (i + 1) & (capacity - 1))
        {
            f(data[i]);
        }
    }

    template<class F>
    void ForEach(const F& f) noexcept
    {
        for (size_t i = begin; i != end; i = (i + 1) & (capacity - 1))
        {
            f(data[i]);
        }
    }

    void Enqueue(size_t count) noexcept
    {
        size_t empty_size = capacity - Size();
        if (count >= empty_size)
        {
            size_t prev_capa = capacity;
            size_t out_size  = count - empty_size;
            size_t total = capacity + out_size;
            capacity <<= (log2(total) - log2(capacity) + 1);
            T* prev_data = data;
            data = new T[capacity];
            size_t end_size = prev_capa - begin;
            memcpy(data, prev_data + begin, end_size);
            memcpy(data + end_size, prev_data, begin);
            delete[] prev_data;
            begin   = 0;
            end     = prev_capa + out_size;
        }
        else
        {
            end = (end + count) & (capacity - 1);
        }
    }

    void Dequeue(size_t count) noexcept
    {
        if (count >= Size())
        {
            begin   = 0;
            end     = 0;
        }
        else
        {
            begin = (begin + count) & (capacity - 1);
        }
    }
};

}

#include <iostream>
#include <benchmark.hpp>
#include <deque>

struct packet
{
    char data[512];
};

int MULTIPLE_QUEUE_TEST()
{
    using namespace Latte;
    size_t count = 1000;
    std::deque<packet> d;
    MultipleQueue<packet> q;;


    srand(488);
    size_t sum = 0;
    size_t max = 0;
    for (size_t i = 0; i < count; ++i)
    {
        sum += rand() % 10000;
        max = (max < sum) ? sum : max;
        auto r = rand() % 10000;
        sum = (sum <= r) ? 0 : sum - r;
    }
    std::cout << "max = " << max << "\n";
    std::cout << "sum = " << sum << "\n";

    srand(488);
    printf("q = %f\n", timer_for_light(count, [&]
    {
        size_t size = rand() % 10000;
        size_t qsize = q.Size();
        q.Enqueue(size);
        size_t c = 0;
        q.ForToEnd(qsize, [&](packet& i) { i.data[0] = c++; });
        size = rand() % 10000;
        qsize = q.Size();
        size = (size <= qsize) ? size : qsize;
        q.ForFromBegin(size, [&](packet& p) 
        { 
            for (int i = 0; i < 512; ++i)
            {
                p.data[i] = i;
            }
        });
        q.Dequeue(size);
        return;
    }));
    std::cout << "q.size = " << q.Size() << "\n";

    size_t count2 = 1000 / 3.5;

    srand(488);
    printf("d = %f\n", timer_for_light(count, [&]
    {
        size_t size = rand() % 10000;
        for (int i = 0; i < size; ++i)
        {
            packet p;
            for (int i = 0; i < 512; ++i)
            {
                p.data[i] = i;
            }
            d.push_back(p);
        }
        size = rand() % 10000;
        for (int i = 0; i < size; ++i)
        {
            if (!d.empty())
            {
                d.front().data[0] = i;
                d.pop_front();
            }
        }
    }));
    std::cout << "d.size = " << d.size() << "\n";

    printf("d.for = %f\n", timer_for_light(count2, [&]
    {
        for (int i = 0; i < d.size(); ++i)
        {
            d[i] = packet{};
        }
    }));

    printf("q.for = %f\n", timer_for_light(count2, [&]
    {
        size_t begin = q.Begin();
        size_t end = q.End();
        for (size_t i = begin; i != end; q.AddIndex(i))
        {
            q.Data(i) = packet{};
        }
    }));

    return 0;
}

#endif

