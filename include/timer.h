// timer.h
#ifndef TIMER_H
#define TIMER_H

#include <errno.h>
#include <limits.h>
#include <sched.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <asm/unistd.h>
#include <linux/perf_event.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <sys/times.h>
#include <sys/types.h>

#define NANOS_PER_SEC   1000000000ULL
#define MICROS_PER_SEC  1000000ULL
#define MILLI_PER_SEC   1000ULL

#define SECONDS_PER_NANO    ((double)1.0 / (double)1000000000ULL)
#define SECONDS_PER_MICRO   ((double)1.0 / (double)1000000ULL)
#define SECONDS_PER_MILLI   ((double)1.0 / (double)1000ULL)

typedef union {
    uint64_t uint64;
    struct {
        uint32_t low;
        uint32_t high;
    } uint32;
} Timer;

typedef struct {
    uint64_t freq;
    uint64_t cycles;
} ClockInfo;

typedef enum {
    TIMER_CLOCK_GETTIME = 0,
    TIMER_RDTSC,
    TIMER_PERF_EVENT
} TimerEvents;

typedef struct {
    TimerEvents type;
    Timer       start;
    Timer       stop;
    ClockInfo   ci;
} TimerDataLB;

int lb_timer_init(TimerEvents type, TimerDataLB* tdata);
void lb_timer_start(TimerDataLB* tdata);
void lb_timer_stop(TimerDataLB* tdata);
void lb_timer_close(TimerDataLB* tdata);

int lb_timer_as_ns(TimerDataLB* tdata, uint64_t* ns);
int lb_timer_as_cycles(TimerDataLB* tdata, uint64_t* cycles);
int lb_timer_as_resolution(TimerDataLB* tdata, uint64_t* resolution);

int lb_timer_supports_cycles(TimerEvents type);
int lb_timer_has_type(TimerEvents type);

int lb_timer_sleep(uint64_t ns);

#endif /* TIMER_H */
