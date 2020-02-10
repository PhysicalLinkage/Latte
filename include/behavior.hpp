
#ifndef BEHAVIOR_HPP
#define BEHAVIOR_HPP

#include <memory>

class State;

class Behavior
{
    friend class State;
    State* state;
protected:
    virtual void Start() = 0;
    virtual void Update() = 0;
    void Transition(std::unique_ptr<Behavior>) noexcept;
};

#endif
