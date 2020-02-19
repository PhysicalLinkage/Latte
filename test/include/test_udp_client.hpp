
#include <udp_client.hpp>

class TestClient : public UDPClient
{
    class TestPacket : public SendPacket
    {
    public:
        explicit TestPacket(std::unique_ptr<std::vector<iovec>> iovs) noexcept
            : SendPacket {std::move(iovs)}
        {}
        ~TestPacket() noexcept
        {
            printf("send end\n");
        }
    };

public:
    explicit TestClient() noexcept
        : UDPClient {4888, "127.0.0.1"}
    {
        static char message[] = "Hello, I'm TestClient.";
        auto iovs = std::make_unique<std::vector<iovec>>(1);
        iovs->front().iov_base = message;
        iovs->front().iov_len = sizeof(message);
        Send(std::make_unique<TestPacket>(std::move(iovs)));
    }
private:
    void OnRecv(std::unique_ptr<Packet>&& packet_) noexcept override
    {
        printf("packet size : %u\n", packet_->size);
        printf("packet data : %s\n", packet_->data);
    }
};

int test_udp_client()
{
    TestClient client;

    for (;;)
    {
        client.RecvUpdate();
        client.SendUpdate();
    }

    return 0;
}
