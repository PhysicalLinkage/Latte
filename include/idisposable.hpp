
#ifndef IDISPOSABLE_HPP
#define IDISPOSABLE_HPP

class IDisposable
{
public:
    virtual void Dispose() = 0;
};

class EmptyDisposable : public IDisposable
{
public:
    void Dispose() noexcept override
    {
    }
};

#endif

