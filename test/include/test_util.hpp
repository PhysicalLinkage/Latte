
#include <util.hpp>
#include <stdio.h>

int test_util()
{
    int cmd;
    const int pow = 0;
    const int log = 1;
    int x;

    while (true)
    {
        printf("pow -> 0, log -> 1, exit -> 2 : ");
        scanf("%d", &cmd);
        printf("x : ");
        scanf("%d", &x);

        if (cmd == pow)
        {
            printf("Pow2(x) = %lu\n", Pow2(x));
        } 
        else 
        if (cmd == log)
        {
            printf("Log2(x) = %lu\n", Log2(x-1)+1);
        }
        else
        {
            printf("bye\n");
            break;
        }
    }

    return 0;
}
