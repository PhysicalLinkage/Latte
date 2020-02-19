
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>


static int SEND_TEST_TEST()
{
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(4888);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    connect(fd, (sockaddr*)&addr, sizeof(addr));
    
    mmsghdr mmhs[8];
    iovec iov;
    static char data[] = "1hello test!";
    data[0] = 6;
    iov.iov_base = data;
    iov.iov_len = sizeof(data);
    for (size_t i = 0; i < 8; ++i)
    {
        auto& mh = mmhs[i].msg_hdr;
        mh.msg_name = NULL;
        mh.msg_namelen = 0;
        mh.msg_iov = &iov;
        mh.msg_iovlen = 1;
        mh.msg_control = NULL;
        mh.msg_controllen = 0;
        mh.msg_flags = 0;
    }

    //sendmmsg(fd, mmhs, 8, 0);
    for (uint8_t i = 0; i < 16; ++i)
    {
        uint8_t type = i & (4 - 1);
        printf("%d\n", type);
    }

    perror("sendmmsg");
    return 0;
}
