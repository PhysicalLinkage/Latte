
#include <client_udp.hpp>

using namespace Client;

class TestClient : public Client::UDP
{
public:
    explicit TestClient() noexcept
        : Client::UDP {4888, "127.0.0.1"}
    {
        static char message[] = "Hello, I'm TestClient.";
        auto iovs = std::make_unique<std::vector<iovec>>(1);
        iovs->front().iov_base = message;
        iovs->front().iov_len = sizeof(message);
        Send(std::make_unique<SendPacket>(std::move(iovs)));
    }
private:
    void OnRecv(std::unique_ptr<Message>&& message) noexcept override
    {
        printf("packet size : %u\n", message->size);
        printf("packet data : %s\n", message->data);
    }
};

int test_client_udp()
{
    TestClient client;

    for (;;)
    {
        client.RecvUpdate();
        client.SendUpdate();
    }

    return 0;
}
