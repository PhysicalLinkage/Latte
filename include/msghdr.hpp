
#include <arpa/inet.h>
#include <iostream>

#define MMU 512

int MSGHDR_TEST()
{
    int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(53548);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    struct msghdr msghdr;
    struct iovec iovecs[2];
    char header[sizeof(uint16_t) * 2] = {0};
    char data[MMU - sizeof(header)] = {0};
    
    msghdr.msg_name = &addr;
    msghdr.msg_namelen = sizeof(addr);
    msghdr.msg_iov = iovecs;
    msghdr.msg_iovlen = 2;
    msghdr.msg_control = NULL;
    msghdr.msg_controllen = 0;
    msghdr.msg_flags = 0;
    iovecs[0].iov_base = header;
    iovecs[0].iov_len = sizeof(header);
    iovecs[1].iov_base = data;
    iovecs[1].iov_len = sizeof(data);

    sendmsg(socket_fd, &msghdr, 0);


    int i;
    std::cin >> i;

    perror("sendmsg");

    std::cout << header << "\n";
    std::cout << data << "\n";

    return 0;
}

