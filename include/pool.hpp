
#ifndef POOL_HPP
#define POOL_HPP

#include <stddef.h>
#include <string.h>
#include <vector>

template<class T>
class Reference;

template<class T>
class Origin
{
    friend class Reference<T>;
private:
    T x;
    size_t ref_count;

    Origin<T> operator=(const Origin<T>& other) = delete;
public:
    explicit Origin() noexcept
        : x {}
        , ref_count {0}
    {

    }

    size_t RefCount() noexcept
    {
        return ref_count;
    }
};

template<class T>
class Pool
{
    friend class Reference<T>;
private:
    std::vector<Origin<T>> origins;

public:
    explicit Pool(size_t capacity) noexcept
        : origins {capacity}
    {

    }

    Reference<T> operator[](size_t i) const noexcept;

    bool IsUsed(size_t i) noexcept
    {
        return (i < origins.size()) ? (0 != origins[i].RefCount()) : false;
    }

    size_t Size() noexcept
    {
        return origins.size();
    }

    Reference<T> New() noexcept;
};

template<class T>
class Reference
{
private:
    Pool<T>* pool;
    size_t index;
public:
    explicit Reference() noexcept
        : pool {nullptr}
        , index {0}
    {
    }

    explicit Reference(Pool<T>* pool_, size_t index_) noexcept
        : pool {pool_}
        , index {index_}
    {
        ++pool->origins[index].ref_count;
    }

    explicit Reference(Reference<T>& other) noexcept
        : Reference{other.pool, other.index}
    {
    }
    
    Reference<T>& operator=(const Reference<T>& other) = delete;

    ~Reference() noexcept
    {
        --pool->origins[index].ref_count;
    }

    size_t Index() noexcept
    {
        return index;
    }

    T& operator*() noexcept
    {
        return pool->origins[index];
    }

    T* operator->() noexcept
    {
        return *pool->origins[index];
    }
};


template<class T>
Reference<T> Pool<T>::New() noexcept
{
    size_t i;
    for (i = 0; i < origins.size(); ++i)
    {
        if (origins[i].RefCount() == 0)
        {
            Reference<T> a(this, i);
            return Reference<T>{this, i};
        }
    }

    origins.resize(i + 1);
    return Reference<T>{this, i};
}


template<class T>
Reference<T> Pool<T>::operator[](size_t i) const noexcept
{
    return Reference<T>{origins[i]};
}

#include <stdio.h>

int POOL_TEST()
{
    Pool<int> f(2);

    const size_t n = 10;
    Reference<int> refs[n];
    for (size_t i = 0; i < n; ++i)
    {
        refs[i] = f.New();
    }
    for (size_t i = 0; i < 4; ++i)
    {
        printf("count %lu\n", refs[i].Index());
        printf("%lu, %d\n", refs[i].Index(), f.IsUsed(refs[i].Index()));
    }

    return 0;
}

#endif

