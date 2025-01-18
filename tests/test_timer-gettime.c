// main
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/utsname.h>

#include "timer.h"

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
    int counter = 0, err = 0;
    do
    {
        uint64_t ns, cycles;
        TimerDataLB timer;
        if ((err = lbtimer_init(TIMER_CLOCK_GETTIME, &timer)) != 0)
        {
            fprintf(stderr, "Timer initialization failed with err %d: %s\n", -err, strerror(-err));
            lbtimer_close(&timer);
            return 0;
        }
        if (counter == 0)
        {
            uint64_t res;
            lbtimer_as_resolution(&timer, &res);
            printf("resolution of clock: %lu\n", res);
        }
        printf("Repetition: %d\n", (counter + 1));
        lbtimer_start(&timer);
        // for (volatile int i = 0; i < 1e9; i++);
        lbtimer_sleep(NANOS_PER_SEC); // 1s
        lbtimer_stop(&timer);
        lbtimer_as_ns(&timer, &ns);
        lbtimer_as_cycles(&timer, &cycles);
        printf("elapsed time: %.10lfs, cycles: %lu, freq: %luHz\n", (double)ns / NANOS_PER_SEC, cycles, timer.ci.freq);
        lbtimer_close(&timer);
        counter++;
    } while (counter < 20);
    return 0;
}
