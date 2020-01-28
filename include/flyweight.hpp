
#ifndef FLYWEIGHT_HPP
#define FLYWEIGHT_HPP

#include <stddef.h>
#include <string.h>

template<class T>
class Flyweight;

template<class T, class... Args>
class Reuseable
{
    friend class Flyweight<T, Args...>;
private:
    bool is_used;
    T x;

public:

    T& operator*() noexcept
    {
        return x;
    }

    T* operator->() noexcept
    {
        return &x;
    }

    void Release() noexcept
    {
        is_used = false;
    }
};



template<class T>
class Flyweight
{
private:
    size_t capacity;
    size_t size;
    Reuseable<T>* data;

public:
    explicit Flyweight(size_t capacity_) noexcept
        : capacity {(capacity == 0) ? 1 : capacity}
        , size {0}
        , data {new Reuseable<T>[capacity]}
    {
    }

    Reuseable<T>& operator[](size_t i) const noexcept
    {
        return data[i];
    }

    bool IsUsed(size_t i) const noexcept
    {
        return (i < size) ? data[i].is_used : false;
    }

    size_t Size() const noexcept
    {
        return size;
    }

    size_t New() noexcept
    {
        size_t i;
        for (i = 0; i < size; ++i)
        {
            if (!data[i].is_used)
            {
                data[i].is_used = true;
                return i;
            }
        }

        if (capacity == size)
        {
            capacity <<= 1;
            Reuseable<T>* prev_data = data;
            data = new Reuseable<T>[capacity];
            memcpy(data, prev_data, size);
            delete[] prev_data;
        }

        ++size;
        data[i].is_used = true;
        return i;
    }
};

#include <stdio.h>

static int FLYWEIGHT_TEST()
{
    Flyweight<int> f(2);

    const size_t n = 10;
    size_t ids[n];
    for (size_t i = 0; i < n; ++i)
    {
        ids[i] = f.New();
    }
    for (size_t i = 0; i < 4; ++i)
    {
        printf("%lu, %d\n", ids[i], f.IsUsed(ids[i]));
    }
    return 0;
}

#endif

