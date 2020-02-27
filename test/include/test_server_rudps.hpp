
#include <server_rudps.hpp>

struct TestPacket : public RUDPS::SendPacket
{
    std::shared_ptr<Message> message;
};

class TestServer : public ServerRUDPS
{
public:
    explicit TestServer() noexcept
        : ServerRUDPS {55488}
    {}
private:
    void OnRecv(
        SRUDPS& rudps,
        iovec& iov,
        std::shared_ptr<Message> message) noexcept override
    {
        auto packet = std::make_unique<TestPacket>();
        printf("recv message : %s\n", (char*)iov.iov_base);
        packet->iovs.resize(1);
        packet->iovs[0].iov_base = iov.iov_base;
        packet->iovs[0].iov_len = iov.iov_len;
        printf("-----------------\n\n");
        packet->message = std::move(message);
        rudps.Send(std::move(packet));
    }
};

int test_server_rudps()
{
    Frame frame;
    TestServer server;

    for (;;)
    {
        frame.Update();
        server.Update(frame);
    }

    return 0;
}


