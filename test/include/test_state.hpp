
#ifndef TEST_STATE_HPP
#define TEST_STATE_HPP

#include <memory>

#include <frame_timer.hpp>

#include <state.hpp>

class TestBehavior1 : public Behavior
{
public:
    explicit TestBehavior1(std::unique_ptr<int> value_) noexcept;
private:
    std::unique_ptr<int> value;
    int life;
    FrameTimer timer;

    void Start() noexcept override;
    void Update() noexcept override;
};

class TestBehavior2 : public Behavior
{
public:
    explicit TestBehavior2(std::unique_ptr<int> value_) noexcept;
private:
    std::unique_ptr<int> value;
    int life;
    FrameTimer timer;

    void Start() noexcept override;
    void Update() noexcept override;
};

int test_state();

#endif

