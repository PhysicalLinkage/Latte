
#include <test_state.hpp>

TestBehavior1::TestBehavior1(std::unique_ptr<int> value_) noexcept
    : value {std::move(value_)}
    , life {4}
    , timer {}
{
    timer.Setup(1e9);
}

void TestBehavior1::Start() noexcept
{
    printf("TestBehavior1\n");
    printf("value : %d\n", *value);
}

void TestBehavior1::Update() noexcept
{
    timer.Update();

    if (timer.IsExpired())
    {
        timer.Reset();
        if (life > 0)
        {
            ++*value;
            printf("%d\n", life);
        }
        else if (life == 0)
        {
            printf("Transition to TestBehavior2\n");
        }
        else
        {
            printf("\n");
            Transition(std::make_unique<TestBehavior2>(std::move(value)));
            return;
        }
        --life;
    }
}

TestBehavior2::TestBehavior2(std::unique_ptr<int> value_) noexcept
    : value {std::move(value_)}
    , life {8}
    , timer {}
{
    timer.Setup(1e9);
}

void TestBehavior2::Start() noexcept
{
    printf("TestBehavior2\n");
    printf("value : %d\n", *value);
}

void TestBehavior2::Update() noexcept
{
    timer.Update();

    if (timer.IsExpired())
    {
        timer.Reset();
        if (life > 0)
        {
            ++*value;
            printf("%d\n", life);
        }
        else if (life == 0)
        {
            printf("Transition to TestBehavior1\n");
        }
        else
        {
            printf("\n");
            Transition(std::make_unique<TestBehavior1>(std::move(value)));
            return;
        }
        --life;
    }
}

int test_state()
{
    State state;

    state.Start(std::make_unique<TestBehavior1>(std::make_unique<int>(0)));

    for (;;)
    {
        Frame.Update();
        state.Update();
    }

    return 0;
}

