// main
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/utsname.h>

#include "timer.h"

#define ABS(x) (((x) < 0) ? -(x) : (x))

void print_arch_info()
{
    struct utsname sysinfo;
    if (uname(&sysinfo) == 0)
    {
        printf("System Name: %s\n", sysinfo.sysname);
        printf("Node Name: %s\n", sysinfo.nodename);
        printf("Release: %s\n", sysinfo.release);
        printf("Version: %s\n", sysinfo.version);
        printf("Machine: %s\n", sysinfo.machine);
        printf("----------\n");
    }
    else
    {
        printf("sysinfo not available\n");
        printf("----------\n");
    }
}

int main()
{
    print_arch_info();
    struct timespec ts0, ts1;
    uint64_t raw_ns_start, raw_ns_end, raw_delta;
    int counter = 0, err = 0;
    do
    {
        uint64_t ns, cycles;
        TimerDataLB timer;
        if ((err = lb_timer_init(TIMER_RDTSC, &timer)) != 0)
        {
            fprintf(stderr, "Timer initialization failed with err %d: %s\n", -err, strerror(-err));
            lb_timer_close(&timer);
            return 0;
        }
        if (counter == 0)
        {
            uint64_t res;
            lb_timer_as_resolution(&timer, &res);
            printf("resolution of clock: %lu\n", res);
        }
        printf("Repetition: %d\n", (counter + 1));

        clock_gettime(CLOCK_MONOTONIC_RAW, &ts0);
        raw_ns_start = ((uint64_t)ts0.tv_sec * NANOS_PER_SEC) + ts0.tv_nsec;
        lb_timer_start(&timer);
        // for (volatile int i = 0; i < 1e9; i++);
        lb_timer_sleep(NANOS_PER_SEC); // 1s
        lb_timer_stop(&timer);
        clock_gettime(CLOCK_MONOTONIC_RAW, &ts1);
        raw_ns_end = ((uint64_t)ts1.tv_sec * NANOS_PER_SEC) + ts1.tv_nsec;
        raw_delta = raw_ns_end - raw_ns_start;
        lb_timer_as_ns(&timer, &ns);
        lb_timer_as_cycles(&timer, &cycles);
        double diff_s  = (double)ns / NANOS_PER_SEC;
        double raw_s   = (double)raw_delta / NANOS_PER_SEC;
        double delta_s = diff_s - raw_s;
        printf("CNT elapsed time: %.10lfs, cycles: %lu, freq: %luHz\n", (double)ns / NANOS_PER_SEC, cycles, timer.ci.freq);
        printf("elapsed time (CLOCK_MONOTONIC_RAW): %.9lf s\n", raw_s);
        printf("absolute time difference (CNT - CLOCK_MONOTONIC_RAW): %.9lf s\n\n", ABS(delta_s));
        lb_timer_close(&timer);
        counter++;
    } while (counter < 20);
    return 0;
}
