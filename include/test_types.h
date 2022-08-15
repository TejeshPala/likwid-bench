/*
 * =======================================================================================
 *      Filename:  stack.h
 *
 *      Description:  The main header defining the test data types
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

#ifndef TEST_TYPES_H
#define TEST_TYPES_H

#include "bstrlib.h"

typedef struct {
    bstring                 name;
    bstring                 description;
    struct bstrList*        options;
} TestConfigParameter;

typedef enum {
    TEST_STREAM_TYPE_SINGLE,
    TEST_STREAM_TYPE_DOUBLE,
    TEST_STREAM_TYPE_INT,
} TestConfigStreamType;

typedef enum {
    TEST_STREAM_ALLOC_TYPE_1DIM = 0,
    TEST_STREAM_ALLOC_PERTHREAD,
} TestConfigStreamFlag;

typedef struct {
    bstring                 name;
    int                     num_dims;
    TestConfigStreamType    type;
    bstring                 btype;
    struct bstrList*        dims;
} TestConfigStream;


typedef struct {
    bstring                 name;
    bstring                 value;
} TestConfigVariable;

typedef struct {
    bstring                 name;
    bstring                 description;
    bstring                 language;
    bstring                 code;
    int                     num_params;
    TestConfigParameter *   params;
    int                     num_streams;
    TestConfigStream *      streams;
    int                     num_constants;
    TestConfigVariable *    constants;
    int                     num_vars;
    TestConfigVariable *    vars;
    int                     num_metrics;
    TestConfigVariable *    metrics;
    struct bstrList*        flags;
} TestConfig;
typedef TestConfig* TestConfig_t;

typedef struct {
    void* ptr;
    int dimsizes[3];
    int offsets[3];
    int dims;
    int flags;
    int id;
    int (*init)(void* ptr, int state, int dims, ...);
    TestConfigStreamType type;
} RuntimeStreamConfig;

typedef struct {
    int num_threads;
    int* cpulist;
    pthread_t *threads;
} RuntimeWorkgroupConfig;

typedef struct {
    int iterations;
    double runtime;
    int num_wgroups;
    RuntimeWorkgroupConfig* wgroups;
} RuntimeConfig;


#endif /* TEST_TYPES_H */
