// topology_hwloc.h
#ifndef TOPOLOGY_HWLOC_H
#define TOPOLOGY_HWLOC_H

#include <errno.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef LIKWID_USE_HWLOC
#include "hwloc.h"

int collect_cpuinfo(hwloc_topology_t topo, RuntimeConfig* runcfg);
void print_cpuinfo(hwloc_topology_t topo);
#endif

#endif /* TOPOLOGY_HWLOC_H */
