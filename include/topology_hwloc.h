// topology_hwloc.h
#ifndef TOPOLOGY_HWLOC_H
#define TOPOLOGY_HWLOC_H

#include <errno.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>

#include "topology.h"
#include "error.h"
#include "bstrlib.h"
#include "bstrlib_helper.h"

#ifdef LIKWIDBENCH_USE_HWLOC
#include "hwloc.h"

extern hwloc_topology_t topo;

int collect_cpuinfo(hwloc_topology_t topo, RuntimeConfig* runcfg);
void print_cpuinfo(hwloc_topology_t topo);
void print_topology(hwloc_topology_t topo);

int cpustr_to_cpulist_hwloc(bstring cpustr, int* list, int length);

#endif

#endif /* TOPOLOGY_HWLOC_H */
