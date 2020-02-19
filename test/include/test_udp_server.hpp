
#include <udp_server.hpp>

class TestServer : public UDPServer
{
    class Disposable : public IDisposable
    {
    public: 
        void Dispose() noexcept override
        {
            printf("send end\n");  
        }
    };

public:
    explicit TestServer() noexcept
        : UDPServer {4888}
    {
    }
private:
    void OnRecv(std::unique_ptr<sockaddr_in>&& address_, std::unique_ptr<Packet>&& packet_) noexcept override
    {
        printf("packet size : %u\n", packet_->size);
        printf("packet data : %s\n", packet_->data);
        static char message[] = "Hello, I'm TestServer.";
        auto iovs = std::make_unique<std::vector<iovec>>(1);
        iovs->front().iov_base = message;
        iovs->front().iov_len = sizeof(message);
        Send(std::move(address_), std::move(iovs), std::make_unique<Disposable>());
    }
};

int test_udp_server()
{
    TestServer server;

    for (;;)
    {
        server.RecvUpdate();
        server.SendUpdate();
    }

    return 0;
}
