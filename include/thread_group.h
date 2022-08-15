/*
 * =======================================================================================
 *      Filename:  stack.h
 *
 *      Description:  The header for managing the worker threads
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

#ifndef THREAD_GROUP_H
#define THREAD_GROUP_H

/*#include <stdlib.h>*/
/*#include <stdio.h>*/
/*#include <string.h>*/
/*#include <unistd.h>*/
#include <stdint.h>

#include <pthread.h>

/*#include <map.h>*/

typedef struct {
    pthread_barrier_t barrier;
    pthread_barrier_attr_t b_attr;
} thread_barrier_t;

typedef enum {
    THREAD_DATA_THREADINIT = 0,
    THREAD_DATA_MAX,
} _thread_data_flags;
#define THREAD_DATA_MIN THREAD_DATA_FLAG_THREADINIT

#define THREAD_DATA_THREADINIT_FLAG (1<<THREAD_DATA_THREADINIT)

typedef struct {
    uint64_t iters;
    uint64_t cycles;
    uint64_t min_runtime;
    //const TestConfig_t test;
    int hwthread;
    int flags;
} _thread_data;
typedef _thread_data* thread_data_t;

typedef struct {
    pthread thread;
    int local_id;
    int global_id;
    double runtime;
    uint64_t cycles;
    thread_barrier_t* barrier;
    thread_data_t data;
} thread_group_thread_t;

typedef struct {
    int id;
    int workgroup;
    int num_threads;
    thread_group_thread_t *threads;
    thread_barrier_t* barrier;
} thread_group_t;

#endif /* THREAD_GROUP_H */
