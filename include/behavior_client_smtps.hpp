
#ifndef BEHAVIOR_CLIENT_SMTPS_HPP
#define BEHAVIOR_CLIENT_SMTPS_HPP

#include <behavior.hpp>
#include <client_smtps.hpp>
#include <frame_timer.hpp>
#include <type_smtps.hpp>

class LoginClientSMTPS : public Behavior<ClientSMTPS>
{
public:

private:
    void Start() noexcept
    {
        printf("LoginSetup\n");

        SetNextLoginRequest();
    }

    void Update() noexcept
    {

    }

    void SetNextLoginRequest() noexcept;
};


class LoginRequestClientSMTPS : public Behavior<ClientSMTPS>
{
private:
    uint8_t type;
    iovec iov[2];
    std::unique_ptr<DHL> dhl;
    std::unique_ptr<int> life;
    
public:
    explicit LoginRequestClientSMTPS(
        std::unique_ptr<DHL> dhl_,
        std::unique_ptr<int> life_) noexcept
        : type {TYPE_SMTPS_LOGIN}
        , iov {0}
        , dhl {dhl_}
        , life {life_}
    {
    }
private:
    void Start() noexcept
    {
        printf("LoginRequest\n");

        auto& msg_hdr = state->msg_hdr;
        msg_hdr.msg_name        = nullptr;
        msg_hdr.msg_namelen     = 0;
        msg_hdr.msg_iov         = iov;
        msg_hdr.msg_iovlen      = 2;
        msg_hdr.msg_control     = nullptr;
        msg_hdr.msg_controllen  = 0;
        msg_hdr.msg_flags       = 0;
        iov[0].iov_base = &type;
        iov[0].iov_len  = sizeof(type);
        iov[1].iov_base = dhl->public_key;
        iov[1].iov_len  = DHL_KEY_SIZE;
    }

    void Update() noexcept
    {
        int len = sendmsg(state->fd, &state->msg_hdr, MSG_DONTWAIT);
        
        if (len == (sizeof(type) + DHL_KEY_SIZE))
        {
            SetNextLoginWait();
            return;
        }
    }

    void SetNextLoginWait() noexcept;
};

class LoginWaitClientSMTPS : public Behavior<ClientSMTPS>
{
private:
    std::unique_ptr<DHL> dhl;
    std::unique_ptr<DHL>
    FrameTimer timer;
public:
    explicit LoginWaitClientSMTPS(std::unique_ptr<DHL> d) noexcept
        : dhl {std::move(d)}
        , timer {}
    {
        
    }
private:

    void Start() noexcept
    {
        printf("LoginWait\n");

        timer.Setup(1e9);
    }

    void Update() noexcept
    {
        size_t len = recv(state->fd, state->data, sizeof(state->data), MSG_DONTWAIT);
        if (len > 0)
        {
            if (len != (sizeof(uint8_t) + DHL_KEY_SIZE))
            {
                fprintf(stderr, "len of reply of Login is different.\n");
                exit(1);
            }

            uint8_t& type = *(uint8_t*)state->data;
            if (type != TYPE_SMTPS_LOGIN)
            {
                fprintf(stderr, "%s is expected but %s is coming.\n",
                    type_smtps_to_string(TYPE_SMTPS_LOGIN),
                    type_smtps_to_string(type));
                exit(1);
            }

            dhl->ComputeKey(state->key, (uint8_t*)(state->data + sizeof(uint8_t)));

        }

        timer.Update(MainFrame);

        if (timer.IsExpired())
        {
            SetNextLoginRequest();
            return;
        }
    }

    void SetNextLoginRequest() noexcept;
    void SetNextBeginRequest() noexcept;
};


class BeginRequestClientSMTPS : public Behavior<ClientSMTPS>
{
    void Start() noexcept
    {
       printf("BeginRequest\n"); 
    }

    void Update() noexcept
    {
        
    }
};


void LoginClientSMTPS::SetNextLoginRequest() noexcept
{
    SetNext(std::make_unique<LoginRequestClientSMTPS>(
                std::make_unique<DHL>(),
                std::make_unique<int>(4)));
}

void LoginRequestClientSMTPS::SetNextLoginWait() noexcept
{
    SetNext(std::make_unique<LoginWaitClientSMTPS>(std::move(dhl)));
}

void LoginWaitClientSMTPS::SetNextLoginRequest() noexcept
{
    SetNext(std::make_unique<LoginRequestClientSMTPS>(std::move(dhl)));
}

void LoginWaitClientSMTPS::SetNextBeginRequest() noexcept
{
    SetNext(std::make_unique<BeginRequestClientSMTPS>());
}

#include <state_machine.hpp>

int BEHAVIOR_CLIENT_SMTPS_TEST()
{
    StateMachine<ClientSMTPS> sm;
    sm.Start(
        std::make_unique<ClientSMTPS>(4888, "127.0.0.1"),
        std::make_unique<LoginClientSMTPS>());

    for (;;)
    {
        MainFrame.Update();
        sm.Update();
    }

    return 0;
}

#endif

