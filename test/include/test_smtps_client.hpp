
#include <smtps_client.hpp>

int test_smtps_client()
{
    SMTPSClient client(4888, "127.0.0.1");
    client.Contact();
    for (;;)
    {
        Frame.Update();
        client.Update();
    }
    return 0;
}

