#ifndef BENCH_H
#define BENCH_H

typedef void (*BenchFuncPrototype)();

int run_benchmark(RuntimeThreadConfig* data);

#endif
