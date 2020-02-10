
#ifndef EPOLL_HPP
#define EPOLL_HPP

#include <sys/epoll.h>
#include <arpa/inet.h>

#include <vector>

template<size_t EVENT_LEN>
class TurnEPOLL
{
private:
    int fd;
    std::vector<epoll_event> events;

    int fd_len;
public:
    explicit TurnEPOLL() noexcept
        : fd {epoll_create1(0)}
        , events {0}
        , fd_len {0}
    {
        
    }

    void Add

    void Update() noexcept
    {

    }
};

#endif

