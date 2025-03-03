/*
 * =======================================================================================
 *
 *      Filename:  topology.h
 *
 *      Description:  Header File for topology module
 *
 *      Version:   <VERSION>
 *      Released:  <DATE>
 *
 *      Author:   Thomas Gruber (tg), thomas.gruber@googlemail.com
 *      Project:  likwid-bench
 *
 *      Copyright (C) 2019 RRZE, University Erlangen-Nuremberg
 *
 *      This program is free software: you can redistribute it and/or modify it under
 *      the terms of the GNU General Public License as published by the Free Software
 *      Foundation, either version 2 of the License, or (at your option) any later
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

#ifndef TOPOLOGY_H
#define TOPOLOGY_H

#include "bstrlib.h"
#include <stdbool.h>

extern struct tagbstring _topology_interesting_flags[];

int check_hwthreads();
int get_num_hw_threads();
int lb_cpustr_to_cpulist(bstring cpustr, int* list, int length);
void destroy_hwthreads();

#ifdef __cplusplus
extern "C" {
#endif

int check_feature_flags(RuntimeConfig* runcfg);
int initialize_cpu_lists(int _max_processor);
int parse_cpu_folders();

#ifdef __cplusplus
}
#endif

#endif /* TOPOLOGY_H */
