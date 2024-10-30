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
/*#include <map.h>*/
#include <stdint.h>
#include <pthread.h>

#include "test_types.h"

#define MIN_ITERATIONS 100
#define MIN_RUNTIME 0.2
#define TIMEOUT_SECONDS 30

int send_cmd(LikwidThreadCommand cmd, RuntimeThreadConfig* thread);
int destroy_threads(int num_wgroups, RuntimeWorkgroupConfig* wgroups);
int update_threads(RuntimeConfig* runcfg);
int create_threads(int num_wgroups, RuntimeWorkgroupConfig* wgroups);
int join_threads(int num_wgroups, RuntimeWorkgroupConfig* wgroups);

#endif /* THREAD_GROUP_H */
