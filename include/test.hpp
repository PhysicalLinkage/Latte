
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>

static int TEST_TEST()
{
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(53548);
    addr.sin_addr.s_addr = inet_addr("153.148.31.174");
    char data[64];

    mmsghdr m;
    iovec iov;
    m.msg_hdr.msg_name = &addr;
    m.msg_hdr.msg_namelen = sizeof(sockaddr);
    m.msg_hdr.msg_iov = &iov;
    m.msg_hdr.msg_iovlen = 1;
    iov.iov_base = data;
    iov.iov_len = 30;
    sendto(fd, data, 30, 0, (sockaddr*)&addr, sizeof(addr));
    perror("sendto");
    return 0;
}
