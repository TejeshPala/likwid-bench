#ifndef RESULTS_H
#define RESULTS_H

#include <map.h>

typedef struct {
    int cpu;
    Map_t results;
} BenchThreadResults;

typedef struct {
    int num_threads;
    BenchThreadResults *threads;
    Map_t formulas;
    Map_t consts;
} BenchResults;


int init_results(int num_threads, int *cpus);
int set_result(int thread, char* name, double value);
int set_result_for_all(char* name, double value);
int add_formula(char* name, char* formula);
int add_const(char* name, double constant);
int get_formula(int thread, char* name, double* value);
void destroy_results();

#endif /* RESULTS_H */
