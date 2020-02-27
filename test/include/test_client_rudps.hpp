
#include <client_rudps.hpp>

int test_client_rudps()
{
    Frame frame;

    std::vector<std::unique_ptr<ClientRUDPS>> clients(4);
    long nanos[] = { (long)2e9, (long)3e9, (long)4e9, (long)5e9 };

    size_t i = 0;
    for (auto& client : clients)
    {
        client = std::make_unique<ClientRUDPS>(4888, "127.0.0.1");
        client->Contact();
        client->InitTimer(nanos[i]);
        printf("%ld\n", nanos[i]);
        ++i;
    }
    
    for (;;)
    {
        frame.Update();
        for (auto& client : clients)
        {
            client->Update(frame);
        }
    }

    return 0;
}
