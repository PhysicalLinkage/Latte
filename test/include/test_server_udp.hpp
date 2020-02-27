
#include <server_udp.hpp>

using namespace Server;

class TestServer : public Server::UDP
{
public:
    explicit TestServer() noexcept
        : Server::UDP {4888}
    {
    }
protected:
    void OnRecv(
            std::unique_ptr<sockaddr_in>&& address,
            std::unique_ptr<Message>&& message) noexcept override
    {
        printf("packet size : %u\n", message->size);
        printf("packet data : %s\n", message->data);
        addr = *address;
        static char msg[] = "Hello, I'm TestServer.";
        auto iovs = std::make_unique<std::vector<iovec>>(1);
        iovs->front().iov_base = msg;
        iovs->front().iov_len = sizeof(msg);
        Send(std::make_unique<SendPacket>(addr, std::move(iovs)));
    }


    sockaddr_in addr;
};

int test_server_udp()
{
    TestServer server;

    for (;;)
    {
        server.RecvUpdate();
        server.SendUpdate();
    }

    return 0;
}
