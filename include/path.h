// path.h
#ifndef PATH_H
#define PATH_H

#include "bstrlib.h"
#include "bstrlib_helper.h"

#ifdef __cplusplus
extern "C" {
#endif

static struct tagbstring bcpuinfo = bsStatic("/proc/cpuinfo");
static struct tagbstring cpu_dir = bsStatic("/sys/devices/system/cpu");

static struct tagbstring bproc_status = bsStatic("/proc/self/status");
static struct tagbstring bproc_stat = bsStatic("/proc/self/stat");

#ifdef __cplusplus
}
#endif

#endif /* PATH_H */
