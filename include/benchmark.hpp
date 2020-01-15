#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/resource.h>
#include <chrono>

double timer_for_heavy(const auto& f)
{
    clock_t start, stop;
    start = clock();
    f();
    stop = clock();
    return (double)(stop - start) / CLOCKS_PER_SEC;
}

double timer_for_light(int count, const auto& f)
{
    return timer_for_heavy([&]
    { 
        for(int i = 0; i < count; ++i)
        {
            f();
        }
    });
}

#ifdef BENCHMARK_TEST
int main()
{
    int count = 1000000;
    struct timeval tv;
    struct rusage r;
    struct tms t;
    struct timespec tp;
    time_t tt;
    struct tm tm;

    printf("clock        : %fs\n", timer_for_light(count, [] {clock();}));
    printf("gettimeofday : %fs\n", timer_for_light(count, [&]{gettimeofday(&tv, NULL);}));
    printf("getrusage    : %fs\n", timer_for_light(count, [&]{getrusage(RUSAGE_SELF, &r);}));
    printf("times        : %fs\n", timer_for_light(count, [&]{times(&t);}));
    printf("clock_gt r   : %fs\n", timer_for_light(count, [&]{clock_gettime(CLOCK_REALTIME, &tp);}));
    printf("clock_gt rc  : %fs\n", timer_for_light(count, [&]{clock_gettime(CLOCK_REALTIME_COARSE, &tp);}));
    printf("clock_gt mc  : %fs\n", timer_for_light(count, [&]{clock_gettime(CLOCK_MONOTONIC_COARSE, &tp);}));
    printf("time         : %fs\n", timer_for_light(count, [] {time(NULL);}));
    printf("gmtime       : %fs\n", timer_for_light(count, [&]{gmtime(&tt);}));
    printf("gmtime_r     : %fs\n", timer_for_light(count, [&]{gmtime_r(&tt, &tm);}));
    printf("chrono       : %fs\n", timer_for_light(count, [] {std::chrono::steady_clock::now();}));

    return 0;
}
#endif
