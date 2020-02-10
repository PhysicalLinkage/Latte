
#ifndef STATE_HPP
#define STATE_HPP

#include <memory>

#include <frame.hpp>

class State;

class Behavior
{
    friend class State;
private:
    State* state;
protected:
    virtual void Start() = 0;
    virtual void Update() = 0;
    void Transition(std::unique_ptr<Behavior> behavior) noexcept;
};


class State
{
    std::unique_ptr<Behavior> behavior;
    std::unique_ptr<Behavior> next_behavior;
public:
    void Start(std::unique_ptr<Behavior>) noexcept;
    void Update() noexcept;
};

void Behavior::Transition(std::unique_ptr<Behavior> behavior_) noexcept
{
    state->Start(std::move(behavior_));
}

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

// test---------------------------------------
#include <frame_timer.hpp>

class TestBehavior2 : public Behavior
{
public:
    explicit TestBehavior2(std::unique_ptr<int> count) noexcept;
    void Start() noexcept override;
    void Update() noexcept override;
};

class TestBehavior1 : public Behavior
{
    std::unique_ptr<int> value;
    FrameTimer timer;
    int life;
    void Start() noexcept override
    {
        printf("TestBehavior1\n");
        printf("value :  %d\n", *value);
    }

    void Update() noexcept override
    {
        timer.Update(MainFrame);

        if (timer.IsExpired())
        {
            timer.Reset();
            if (life > 0)
            {
                printf("%d\n", life);
            }
            else if (life == 0)
            {
                *value += 4;
                printf("Move to TestBehavior2\n\n");
            }
            else
            {
                SetNext2();
                return;
            }
            --life;
        }
    }

    void SetNext2() noexcept;

public:
    explicit TestBehavior1(std::unique_ptr<int> value_) noexcept
        : value {std::move(value_)}
        , timer {}
        , life {4}
     {
         timer.Setup(1e9);
     }
};

class TestBehavior2 : public Behavior
{
    std::unique_ptr<int> value;
    FrameTimer timer;
    int life;
    void Start() noexcept override
    {
        printf("TestBehavior2\n");
        printf("value :  %d\n", *value);
    }

    void Update() noexcept override
    {
        timer.Update(MainFrame);

        if (timer.IsExpired())
        {
            timer.Reset();
            if (life > 0)
            {
                printf("%d\n", life);
            }
            else if (life == 0)
            {
                *value += 8;
                printf("Move to TestBehavior1\n\n");
            }
            else
            {
                SetNext1();
                return;
            }
            --life;
        }
    }

    void SetNext1() noexcept;
public:
    explicit TestBehavior2(std::unique_ptr<int> value_) noexcept
        : value {std::move(value_)}
        , timer {}
        , life {8}
    {
        timer.Setup(1e9);
    }
};

void TestBehavior1::SetNext2() noexcept
{
    Transition(std::make_unique<TestBehavior2>(std::move(value)));
}

void TestBehavior2::SetNext1() noexcept
{
    Transition(std::make_unique<TestBehavior1>(std::move(value)));
}

int STATE_MACHINE_TEST()
{
    State state;

    state.Start(std::make_unique<TestBehavior1>(std::make_unique<int>(0)));

    while (true)
    {
        MainFrame.Update();
        state.Update();
    }

    return 0;
}

#endif

