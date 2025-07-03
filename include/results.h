/*
 * =======================================================================================
 *      Filename:  results.h
 *
 *      Description:  The header for the per-thread result storage
 *
 *      Version:   <VERSION>
 *      Released:  <DATE>
 *
 *      Author:  Thomas Gruber (tg), thomas.roehl@gmail.com
 *      Project:  likwid-bench
 *
 *      Copyright (C) 2019 RRZE, University Erlangen-Nuremberg
 *
 *      This program is free software: you can redistribute it and/or modify it under
 *      the terms of the GNU General Public License as published by the Free Software
 *      Foundation, either version 3 of the License, or (at your option) any later
 *      version.
 *
 *      This program is distributed in the hope that it will be useful, but WITHOUT ANY
 *      WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 *      PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License along with
 *      this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * =======================================================================================
 */

#ifndef RESULTS_H
#define RESULTS_H

#ifndef WITH_BSTRING
#define WITH_BSTRING
#endif
#include "map.h"
#include "test_types.h"
#include "bstrlib.h"

typedef struct {
    int cpu;
    Map_t results;
    Map_t consts;
} BenchThreadResults;

typedef struct {
    int num_threads;
    BenchThreadResults *threads;
    Map_t formulas;
    Map_t consts;
} BenchResults;


int init_results(int num_threads, int *cpus);
int set_result(int thread, bstring name, double value);
int set_result_for_all(bstring name, double value);
int add_formula(bstring name, bstring formula);
int add_const(bstring name, double constant);
int add_thread_const(int thread, bstring name, double constant);
int get_formula(int thread, bstring name, double* value);
void destroy_results();

int init_result(RuntimeWorkgroupResult* result);
int add_value(RuntimeWorkgroupResult* result, bstring name, double value);
int update_value(RuntimeWorkgroupResult* result, bstring name, double value);
int add_variable(RuntimeWorkgroupResult* result, bstring name, bstring value);
int update_variable(RuntimeWorkgroupResult* result, bstring name, bstring value);
int get_value(RuntimeWorkgroupResult* result, bstring name, double* value);
int get_variable(RuntimeWorkgroupResult* result, bstring name, size_t* value);
int replace_all(RuntimeWorkgroupResult* result, bstring formula, struct bstrList* exclude);
void destroy_result(RuntimeWorkgroupResult* result);
void print_result(RuntimeWorkgroupResult* result);

int fill_results(RuntimeConfig* runcfg);
#endif /* RESULTS_H */
