#ifndef BENCH_H
#define BENCH_H

typedef void (*BenchFuncPrototype)();

void* run_benchmark(void* arg);

#endif
