/*
 * =======================================================================================
 *      Filename:  stack.h
 *
 *      Description:  The header for managing a group of worker threads
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

#ifndef WORKGROUP_H
#define WORKGROUP_H

#include "test_types.h"
#include "bstrlib.h"

void delete_workgroup(RuntimeWorkgroupConfig* wg);
void release_streams(int num_wgroups, RuntimeWorkgroupConfig* wgroups);
int resolve_workgroup(RuntimeWorkgroupConfig* wg, int maxThreads);
int resolve_workgroups(int num_wgroups, RuntimeWorkgroupConfig* wgroups);
int manage_streams(RuntimeWorkgroupConfig* wg, RuntimeConfig* runcfg);
void print_workgroup(RuntimeWorkgroupConfig* wgroup);
void update_results(int num_wgroups, RuntimeWorkgroupConfig* wgroups);


#endif /* WORKGROUP_H */
