
#ifndef MTCP_HPP
#define MTCP_HPP


#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>

class TCP
{
private:
    int socket_fd;
    int client_fd;
    struct sockaddr_in client_addr;
public:
    explicit TCP(int a = 1) noexcept
        : socket_fd {socket(AF_INET, SOCK_STREAM, 0)}
        , client_fd {-1}
        , client_addr {}
    {
    }

    ~TCP() noexcept
    {
        close(socket_fd);
        close(client_fd);
    }

    void Bind(uint16_t port) noexcept
    {
        struct sockaddr_in addr_in;
        addr_in.sin_family = AF_INET;
        addr_in.sin_port = htons(port);
        addr_in.sin_addr.s_addr = INADDR_ANY;
    
        if (bind(socket_fd, (sockaddr*)&addr_in, sizeof(addr_in)) == -1)
        {
            perror("bind");
            exit(1);
        }
    }

    void Listen(int backlog) noexcept
    {
        listen(socket_fd, backlog);
    }

    void Accept() noexcept
    {
        {
            socklen_t len = sizeof(sockaddr_in);
            client_fd = accept(socket_fd, (sockaddr*)&client_addr, &len);
        }
    }

    void Connect(uint16_t port, const char* addr) noexcept
    {
        struct sockaddr_in addr_in;
        addr_in.sin_family = AF_INET;
        addr_in.sin_port = htons(port);
        addr_in.sin_addr.s_addr = inet_addr(addr);
        
        if (connect(socket_fd, (sockaddr*)&addr_in, sizeof(addr_in)) == -1)
        {
            perror("connect");
            exit(1);
        }

        client_fd = 0;
    }

    size_t Write(const void* data, size_t len) noexcept
    {
        return write(client_fd, data, len);
    }

    size_t Read(void* data, size_t len) noexcept
    {
        return read(client_fd, data, len);
    }
};

static uint8_t msg[10];

int MTCP_TEST()
{
    int sd;
    struct sockaddr_in addr;

    // IPv4 TCP �Υ����åȤ��������
    if((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return -1;
    }

    // �����襢�ɥ쥹�ȥݡ����ֹ�����ꤹ��
    addr.sin_family = AF_INET;
    addr.sin_port = htons(4889);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // ��������³��TCP �ξ��ϡ���³���Ω����ɬ�פ������
    connect(sd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));

    // �ѥ��åȤ� TCP ������
    if(send(sd, "I am send process", 17, 0) < 0) {
        perror("send");
        return -1;
    }

    close(sd);

    return 0;
}


#endif
