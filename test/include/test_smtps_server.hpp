
#include <smtps_server.hpp>

class MySMTPS : public SMTPS
{
public:
    void OnRecv(
            std::unique_ptr<std::vector<iovec>>&& iovs, 
            std::unique_ptr<UDPServer::Packet>&& packet) noexcept override
    {
        for (int i = iovs->size() - 1; i >= 0; --i)
        {
            printf("%s\n", (char*)(*iovs)[i].iov_base);
        }
    }
};

int test_smtps_server()
{
    Frame frame;
    SMTPSServer<MySMTPS> server(4888);
    for (;;)
    {
        frame.Update();
        server.Update(frame);
    }

}

