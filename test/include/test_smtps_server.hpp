
#include <smtps_server.hpp>

class MySMTPS : public SMTPS
{
public:
    void OnRecv(std::unique_ptr<iovec>&& iov) noexcept
    {
       printf("my smtps\n");
    }
};

int test_smtps_server()
{
    SMTPSServer<MySMTPS> server(4888);
    for (;;)
    {
        Frame.Update();
        server.Update();
    }

}

