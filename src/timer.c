#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sched.h>
#include <unistd.h>
#include <asm/unistd.h>
#ifdef __linux__
#include <linux/version.h>
#endif
#include <linux/perf_event.h>
#include <linux/hw_breakpoint.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <sys/types.h>

#include "error.h"
#include "timer.h"

#ifdef __linux__
#define gettid() syscall(SYS_gettid)
#else
#define gettid() getpid()
#endif


#if defined(__x86__64) || defined(__i386__) || defined(__x86_64__)
static inline void _cpuid(uint32_t reg[4], uint32_t leaf, uint32_t subleaf)
{
    __asm__ __volatile__("cpuid"
                         : "=a"(reg[0]), "=b"(reg[1]), "=c"(reg[2]), "=d"(reg[3])
                         : "a"(leaf), "c"(subleaf)
                        );
}

static int _has_rdtscp(void)
{
    static int has_rdtscp = -1;
    if (has_rdtscp == -1)
    {
        uint32_t reg[4];
        _cpuid(reg, 0x80000001, 0);
        has_rdtscp = (reg[3] & (1 << 27)) ? 1 : 0; // RDTSCP check
    }
    return has_rdtscp;
}

static inline void _serialize(void)
{
    uint32_t reg[4];
    _cpuid(reg, 0, 0);
}

static inline uint64_t _rdtsc()
{
    uint32_t lo, hi;
    __asm__ __volatile__("mfence\n\t"
                         "lfence\n\t"
                         "rdtsc\n\t"
                         "lfence\n\t": "=a" (lo), "=d" (hi) : : "memory");
    return ((uint64_t)hi << 32) | lo;
}

static inline uint64_t _rdtscp()
{
    uint32_t lo, hi, aux;
    __asm__ __volatile__("rdtscp" : "=a" (lo), "=d" (hi), "=c" (aux) : : "memory");
    return ((uint64_t)hi << 32) | lo;
}

static inline uint64_t _rd_tsc()
{
    if (_has_rdtscp())
    {
        return _rdtscp();
    }
    else
    {
        _serialize();
        return _rdtsc();
    }
}

#elif defined(__aarch64__) || defined(__arm__)
#include <arm_neon.h>
static inline uint64_t _rd_cf(void)
{
    uint64_t cf;
    __asm__ __volatile__("dsb ish; isb; mrs %0, cntfrq_el0" : "=r" (cf) : : "memory");
    return cf;
}

static inline uint64_t _rd_ct(void)
{
    uint64_t ct;
    __asm__ __volatile__("dsb ish; isb; mrs %0, cntvct_el0" : "=r" (ct) : : "memory");
    return ct;
}

#elif defined(__powerpc) || defined(__ppc__) || defined(__PPC__)
#include <altivec.h>
#include <sys/platform/ppc.h>
static inline uint64_t _rd_timebase()
{
    uint32_t tbl, tbu0, tbu1;
    __asm__ __volatile__("mftbu %0" : "=r"(tbu0) : : "memory");
    __asm__ __volatile__("mftb %0" : "=r"(tbl) : : "memory");
    __asm__ __volatile__("mftbu %0" : "=r"(tbu1) : : "memory");
    if (tbu0 != tbu1)
    {
        __asm__ __volatile__("mftb %0" : "=r"(tbl) : : "memory");
    }
    return (((uint64_t)tbu1) << 32) | tbl;
}
#endif


static clockid_t _get_clockid()
{
#if defined(__x86__64) || defined(__i386__) || defined(__x86_64__)
    return CLOCK_MONOTONIC;
#elif defined(__aarch64__) || defined(__arm__)
    return CLOCK_MONOTONIC;
#elif defined(__powerpc) || defined(__ppc__) || defined(__PPC__)
    return CLOCK_MONOTONIC;
#else
    printf("Using CLOCK_REALTIME as CLOCK_MONOTONIC is unavailable on the architecture!\n");
    return CLOCK_REALTIME;
#endif
}


static uint64_t _get_time_in_ns(void)
{
    struct timespec ts;
    clockid_t clockid = _get_clockid();
    if (clock_gettime(clockid, &ts) != 0)
    {
        ERROR_PRINT(clock_gettime failed with err %d: %s, errno, strerror(errno));
        return 0; // exit(EXIT_FAILURE);
    }
    uint64_t ns = (uint64_t)(ts.tv_sec * NANOS_PER_SEC + ts.tv_nsec);
    return ns;
}

int lb_timer_as_resolution(TimerDataLB* tdata, uint64_t* resolution)
{
    if (!tdata || !resolution)
    {
        return -EINVAL;
    }
    struct timespec res;
    clockid_t clockid = _get_clockid();
    if (clock_getres(clockid, &res) == -1)
    {
        return -errno;
    }
    *resolution = (uint64_t)(res.tv_sec * NANOS_PER_SEC + res.tv_nsec);
    return 0;
}

static inline uint64_t _rd_counter(TimerDataLB* tdata)
{
    if (!tdata)
    {
        return 0ULL;
    }
    switch(tdata->type)
    {
        case TIMER_CLOCK_GETTIME:
            return _get_time_in_ns();
        case TIMER_RDTSC:
#if defined(__x86__64) || defined(__i386__) || defined(__x86_64__)
            return _rd_tsc();
#elif defined(__aarch64__) || defined(__arm__)
            return _rd_ct();
#elif defined(__powerpc) || defined(__ppc__) || defined(__PPC__)
            return _rd_timebase();
#endif
            return 0ULL;
        default:
            return 0ULL;
    }
    return 0ULL;
}

static inline uint64_t _rd_freq(TimerDataLB* tdata)
{
    if (!tdata)
    {
        return 0ULL;
    }
    static uint64_t freq = 0ULL;
    uint64_t sum = 0ULL;
    const int iters = 10;
    uint64_t measure[iters];
    uint64_t start = 0ULL;
    uint64_t end = 0ULL;
    if (freq == 0ULL)
    {
        switch(tdata->type)
        {
            case TIMER_CLOCK_GETTIME:
                freq = 0ULL;
                break;
            case TIMER_RDTSC:
                for (int i = 0; i < iters; i++)
                {
#if defined(__x86__64) || defined(__i386__) || defined(__x86_64__)
                    start = _rd_tsc();
                    lb_timer_sleep(NANOS_PER_SEC);
                    end = _rd_tsc();
                    measure[i] = end - start;
#elif defined(__aarch64__) || defined(__arm__)
                    measure[i] = _rd_cf();
#elif defined(__powerpc) || defined(__ppc__) || defined(__PPC__)
                    start = _rd_timebase();
                    lb_timer_sleep(NANOS_PER_SEC);
                    end = _rd_timebase();
                    measure[i] = end - start;
#endif
                    sum += measure[i];
                }
                freq = (double)sum / iters;
                break;
        }
    }
    return freq;
}

int lb_timer_init(TimerEvents type, TimerDataLB* tdata)
{
    if (!tdata)
    {
        return -EINVAL;
    }
    int err = 0;
    memset(tdata, 0ULL, sizeof(TimerDataLB));
    tdata->type              = type;
    tdata->ci.freq = _rd_freq(tdata);
    err = (tdata->ci.freq == 0ULL) ? 0 : ((tdata->ci.freq > 0ULL) ? 0 : -ENOTSUP);
    return err;
}

void lb_timer_start(TimerDataLB* tdata)
{
    tdata->start.uint64 = _rd_counter(tdata);
}

void lb_timer_stop(TimerDataLB* tdata)
{
    tdata->stop.uint64 = _rd_counter(tdata);
}

int lb_timer_as_ns(TimerDataLB* tdata, uint64_t *ns)
{
    if (!tdata || !ns)
    {
        return -EINVAL;
    }
    uint64_t diff = 0ULL;
    if (tdata->stop.uint64 <= 0ULL && tdata->start.uint64 <= 0ULL && (tdata->stop.uint64 <= tdata->start.uint64))
    {
        ERROR_PRINT(Invalid time err %d: %s, errno, strerror(errno));
        return -errno;
    }
    diff = tdata->stop.uint64 - tdata->start.uint64;
    // printf("time diff: %llu, stop: %llu, start: %llu\n", diff, tdata->stop.uint64, tdata->start.uint64);
    switch(tdata->type)
    {
        case TIMER_CLOCK_GETTIME:
            *ns = diff;
            return 0;
        case TIMER_RDTSC:
            if (tdata->ci.freq != 0ULL)
            {
                *ns = (double) diff * NANOS_PER_SEC / tdata->ci.freq;
                return 0;
            }
            else
            {
                ERROR_PRINT(Not Supported %d: %s, errno, strerror(errno));
                return -ENOTSUP;
            }
        default:
            return -EINVAL;
    }
    return 0;
}

int lb_timer_as_cycles(TimerDataLB* tdata, uint64_t* cycles)
{
    if (!tdata || !cycles)
    {
        return -EINVAL;
    }
    uint64_t diff = 0ULL;
    switch(tdata->type)
    {
        case TIMER_CLOCK_GETTIME:
            *cycles = diff;
            return -ENOTSUP;
        case TIMER_RDTSC:
            if (tdata->stop.uint64 > 0ULL && tdata->start.uint64 > 0ULL && (tdata->stop.uint64 > tdata->start.uint64))
            {
                diff = tdata->stop.uint64 - tdata->start.uint64;
                // printf("cycles diff: %llu, stop: %llu, start: %llu\n", diff, tdata->stop.uint64, tdata->start.uint64);
                *cycles = diff;
                return 0;
            }
            else
            {
                return -EINVAL;
            }
        default:
            return -EINVAL;
    }
    return 0;
}

int lb_timer_supports_cycles(TimerEvents type)
{
    return (type == TIMER_RDTSC);
}

int lb_timer_sleep(uint64_t nanoseconds)
{
    int err = -1;
    struct timespec ts, rem;
    ts.tv_sec = nanoseconds / NANOS_PER_SEC;
    ts.tv_nsec = nanoseconds % NANOS_PER_SEC;
    // printf("sleep for %lfs\n", (nanoseconds * SECONDS_PER_NANO + nanoseconds % SECONDS_PER_NANO));
    clockid_t clockid = _get_clockid();
    while ((err = clock_nanosleep(clockid, 0, &ts, &rem)) == EINTR)
    {
        ts = rem;
    }
    if (errno != 0 && errno != EINTR)
    {
        err = errno;
        ERROR_PRINT(clock_nanosleep failed with error %d: %s, err, strerror(err));
    }
    return err;
}

void lb_timer_close(TimerDataLB* tdata)
{
    tdata->type              = TIMER_CLOCK_GETTIME;
    memset(tdata, 0ULL, sizeof(TimerDataLB));
}
