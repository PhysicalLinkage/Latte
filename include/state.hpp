
#ifndef STATE_HPP
#define STATE_HPP

#include <memory>

#include <behavior.hpp>

class State
{
    std::unique_ptr<Behavior> behavior;
    std::unique_ptr<Behavior> next_behavior;

public:
    void Start(std::unique_ptr<Behavior>) noexcept;
    void Update() noexcept;
};


#endif

