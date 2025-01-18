#ifndef BENCH_H
#define BENCH_H

#include <test_types.h>

typedef void (*BenchFuncPrototype)();

int run_benchmark(RuntimeThreadConfig* data);

#endif
