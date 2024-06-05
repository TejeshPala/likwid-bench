// path.h
#ifndef PATH_H
#define PATH_H

#include "bstrlib.h"
#include "bstrlib_helper.h"

#ifdef __cplusplus
extern "C" {
#endif

struct tagbstring bcpuinfo = bsStatic("/proc/cpuinfo");
struct tagbstring cpu_dir = bsStatic("/sys/devices/system/cpu");

#ifdef __cplusplus
}
#endif

#endif /* PATH_H */
