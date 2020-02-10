
#include <state.hpp>

#include <behavior.hpp>

void Behavior::Transition(std::unique_ptr<Behavior> behavior_) noexcept
{
    state->Start(std::move(behavior_));
}

