
#ifndef BENCHMARK_HPP
#define BENCHMARK_HPP

#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/resource.h>
#include <chrono>

double timer_for_heavy(const auto& f)
{
    struct timeval start, stop;
    gettimeofday(&start, NULL);
    f();
    gettimeofday(&stop, NULL);
    return stop.tv_sec - start.tv_sec + (double)(stop.tv_usec - start.tv_usec) / 1000000;;
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

int BENCHMARK_TEST()
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

    uint64_t micro;

    auto f = [&]
    {
        clock_gettime(CLOCK_REALTIME, &tp);
        micro = tp.tv_sec * 1e+6 + tp.tv_nsec * 1e-9;
    };

    printf("micro       : %fs\n", timer_for_light(count, f));

    return 0;
}

#endif

