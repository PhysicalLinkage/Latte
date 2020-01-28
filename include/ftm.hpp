
#ifndef FTM_HPP
#define FTM_HPP

#include <iostream>
#include <string>
#include <stdint.h>
#include <vector>
#include <string.h>

namespace Latte
{

namespace FTM
{

class Serializer
{
private:
    uint8_t*    bytes;
    size_t    index;

public:
    explicit Serializer(uint8_t* bs, size_t offset = 0) noexcept
        : bytes {bs}
        , index {offset}
    {
    }

    uint8_t* Bytes() noexcept
    {
        return bytes;
    }

    size_t Size() const noexcept
    {
        return index;
    }

    template<typename T>
    Serializer& AddBytes(uint8_t* bs, T size) noexcept
    {
        Add(size);
        memcpy(bytes, bs, size);
        index += size;
        return *this;
    }

    template<typename T>
    Serializer& Add(const T& x) noexcept
    {
        *(T*)(bytes + index) = x;
        index += sizeof(T);
        return *this;
    }

    template<typename T>
    auto AddRange(size_t count) noexcept
    {
        return [&](const auto& on) noexcept
        {
            for (size_t i = 0; i < count; ++i)
            {
                on(i, *(T*)(bytes + index));
                index += sizeof(T);
            }
            return *this;
        };
    }

    template<typename T>
    Serializer& AddString(const std::string& s) noexcept
    {
        const auto f = [&](size_t i, char& c) { c = s[i]; };

        Add<T>(s.size());
        AddRange<char>(s.size())(f);
        return *this;
    }
};

class Deserializer
{
private:
    const uint8_t* const bytes;
    size_t size;
    size_t index;

public:
    explicit Deserializer(const uint8_t* bs, size_t size_, size_t offset = 0) noexcept
        : bytes {bs}
        , size {size_}
        , index {offset}
    {
    }

    size_t Index() const noexcept
    {
        return index;
    }

    bool IsEnd() const noexcept
    {
        return size == index;
    }

    size_t EmptySize() const noexcept
    {
        return size - index;
    }

    bool forward(size_t size) noexcept
    {
        if (EmptySize() < size)
            return false;

        index += size;
        return true;
    }

    bool Bytes(const uint8_t** ref, size_t size_) noexcept
    {
        if (EmptySize() < size_)
            return false;

        *ref = &bytes[index];
        index += size_;
        return true;
    }

    template<typename T>
    bool To(T& x) noexcept
    {
        constexpr size_t size_ = sizeof(T);
        if (EmptySize() < size_)
            return false;

        x = *(T*)(bytes + index);
        index += size_;
        return true;
    }

    template<typename T>
    auto ToRange(size_t count) noexcept
    {
        return [&](const auto& on) noexcept
        {
            for (size_t i = 0; i < count; ++i)
            {
                const T& x = *(T*)(bytes + index);
                on(i, x);
                index += sizeof(T);
            }
        };
    }       

    template<typename T>
    bool ToString(std::string& s) noexcept
    {
        T size;
        if (!To<T>(size))
            return false;

        s.reserve(size);
        for (int i = 0; i < size; ++i)
        {
            char c;
            if (!To<char>(c))
                return false;

            s.push_back(c);
        }
        return true;
    }
};

}

}


int FTM_TEST()
{
    uint8_t bytes[512];

    Latte::FTM::Serializer seri(bytes);
    seri.Add<char>('A')
        .Add<uint16_t>(535)
        .Add<int32_t>(-488)
        .Add<float>(4.8)
        .AddString<uint16_t>("Hello world!!");

    Latte::FTM::Deserializer de(seri.Bytes(), seri.Size());
    char d1;
    uint16_t d2;
    uint32_t d3;
    float d4;
    uint16_t d5;
    const uint8_t* d6;
    if ((!de.To(d1)) ||
        (!de.To(d2)) ||
        (!de.To(d3)) ||
        (!de.To(d4)) ||
        (!de.To(d5)) ||
        (!de.Bytes(&d6, d5)))
        return 1;

    std::cout << "d1 = " << d1 << "\n";
    std::cout << "d2 = " << d2 << "\n";
    std::cout << "d3 = " << d3 << "\n";
    std::cout << "d4 = " << d4 << "\n";
    std::cout << "d5 = " << d5 << "\n";
    std::cout << "d6 = " << d6 << "\n";
        
    return 0;
}

#endif

