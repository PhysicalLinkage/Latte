
#include <state.hpp>

void State::Start(std::unique_ptr<Behavior> behavior_) noexcept
{
    next_behavior = std::move(behavior_);
}


void State::Update() noexcept
{
    if (next_behavior)
    {
        behavior = std::move(next_behavior);
        behavior->state = this;
        behavior->Start();
    }

    behavior->Update();
}


