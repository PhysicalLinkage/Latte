
#ifndef REF_PTR_HPP
#define REF_PTR_HPP

#include <memory>

template<class T>
class ref_ptr
{
public:
    explicit ref_ptr() noexcept
        : ptr {nullptr}
    {
    }

    explicit ref_ptr(T& ptr_) noexcept
        : ptr {&ptr_}
    {
    }

    explicit ref_ptr(ref_ptr<T>& other_) noexcept
        : ptr {other_.ptr}
    {
    }


    explicit ref_ptr(std::unique_ptr<T>& unique_ptr_) noexcept
        : ptr {unique_ptr_.get()}
    {
    }

    ref_ptr<T>& operator=(T& other_) noexcept
    {
        ptr = &other_;
        return *this;
    }

    ref_ptr<T>& operator=(ref_ptr<T>& other_) noexcept
    {
        ptr = other_.ptr;
        return *this;
    }

    ref_ptr<T>& operator=(std::unique_ptr<T>& other_) noexcept
    {
        ptr = other_.get();
        return *this;
    }

    T& operator*() noexcept
    {
        return *ptr;
    }

    T* operator->() noexcept
    {
        return ptr;
    }

    const T& operator*() const noexcept
    {
        return *ptr;
    }

    const T* operator->() const noexcept
    {
        return ptr;
    }

private:
    T* ptr;
};

#endif

