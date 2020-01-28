
#include <mudp.hpp>

#include <iostream>

#define MMU 512

int MSGHDR_TEST()
{
    int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in a;
    a.sin_family = AF_INET;
    a.sin_port = htons(53548);
    a.sin_addr.s_addr = INADDR_ANY;
    bind(socket_fd, (struct sockaddr*)&a, sizeof(a));

    struct msghdr msghdr;
    struct sockaddr addr;
    struct iovec iovecs[2];
    char header[sizeof(uint16_t) * 2] {0};
    char data[MMU - sizeof(header)] = {0};
    
    msghdr.msg_name = &addr;
    msghdr.msg_namelen = sizeof(addr);
    msghdr.msg_iov = iovecs;
    msghdr.msg_iovlen = 2;
    iovecs[0].iov_base = header;
    iovecs[0].iov_len = sizeof(header);
    iovecs[1].iov_base = data;
    iovecs[1].iov_len = sizeof(data);

    recvmsg(socket_fd, &msghdr, 0);

    std::cout << header << "\n";
    std::cout << data << "\n";

    return 0;
}

