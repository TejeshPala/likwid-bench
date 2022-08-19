#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <error.h>

#include "bstrlib.h"
#include "bstrlib_helper.h"
#include "test_types.h"

#include "workgroups.h"
#include "topology.h"
#include <pthread.h>


int resolve_workgroups(int num_wgroups, RuntimeWorkgroupConfig* wgroups)
{
    int hwthreads = get_num_hw_threads();
    if (hwthreads <= 0)
    {
        return -EINVAL;
    }
    
    for (int i = 0; i < num_wgroups; i++)
    {
        RuntimeWorkgroupConfig* wg = &wgroups[i];
        wg->num_threads = 0;
        wg->cpulist = malloc(hwthreads * sizeof(int));
        if (!wg->cpulist)
        {
            return -ENOMEM;
        }
        
        int nthreads = cpustr_to_cpulist(wg->str, wg->cpulist, hwthreads);
        if (nthreads < 0)
        {
            free(wg->cpulist);
            wg->cpulist = NULL;
            return nthreads;
        }
        wg->num_threads = nthreads;
    }
    return 0;
}

void print_workgroup(RuntimeWorkgroupConfig* wg)
{
    printf("Workgroup '%s' with %d hwthreads: ", bdata(wg->str), wg->num_threads);
    if (wg->num_threads > 0)
        printf("%d", wg->cpulist[0]);
    for (int i = 1; i < wg->num_threads; i++)
    {
        printf(", %d", wg->cpulist[i]);
    }
    printf("\n");
}

/*int create_workgroups(RuntimeConfig* cfg, struct bstrList* workgroups)*/
/*{*/
/*    int ret = 0;*/
/*    if (cfg && workgroups && workgroups->qty > 0)*/
/*    {*/
/*        topology_init();*/
/*        CpuTopology_t topo = get_cpuTopology();*/
/*        cfg->wgroups = malloc(workgroups->qty * sizeof(RuntimeWorkgroupConfig));*/
/*        cfg->num_wgroups = 0;*/
/*        int tcount = topo->numHWThreads;*/
/*        for (int i = 0; i < workgroups->qty; i++)*/
/*        {*/
/*            int* tmp = malloc(tcount*sizeof(int));*/
/*            if (tmp)*/
/*            {*/
/*                int nthreads = cpustr_to_cpulist(bdata(workgroups->entry[i]), tmp, tcount);*/
/*                cfg->wgroups[i].cpulist = tmp;*/
/*                cfg->wgroups[i].num_threads = nthreads;*/
/*                if (global_verbosity > DEBUGLEV_ONLY_ERROR)*/
/*                {*/
/*                    bstring dstr = bformat("Workgroup '%s': ", bdata(workgroups->entry[i]));*/
/*                    for (int j = 0; j < nthreads; j++)*/
/*                    {*/
/*                        bstring t = bformat("%d ", cfg->wgroups[i].cpulist[j]);*/
/*                        bconcat(dstr, t);*/
/*                        bdestroy(t);*/
/*                    }*/
/*                    DEBUG_PRINT(DEBUGLEV_INFO, %s, bdata(dstr));*/
/*                }*/
/*                cfg->num_wgroups++;*/
/*            }*/
/*            else*/
/*            {*/
/*                for (int j = i-1; j >= 0; j--)*/
/*                {*/
/*                    free(cfg->wgroups[i].cpulist);*/
/*                    cfg->wgroups[i].cpulist = NULL;*/
/*                    cfg->wgroups[i].num_threads = 0;*/
/*                }*/
/*                free(cfg->wgroups);*/
/*                cfg->wgroups = NULL;*/
/*                cfg->num_wgroups = 0;*/
/*                ret = -ENOMEM;*/
/*                break;*/
/*            }*/
/*        }*/
/*        topology_finalize();*/
/*    }*/
/*    return ret;*/
/*}*/


/*int parse_workgroup(bstring wgroup, RuntimeWorkgroupConfig* config)*/
/*{*/
/*    topology_init();*/
/*    CpuTopology_t topo = get_cpuTopology();*/
/*    int tcount = topo->numHWThreads;*/
/*    int* tmp = malloc(tcount*sizeof(int));*/
/*    if (!tmp)*/
/*    {*/
/*        return -ENOMEM;*/
/*    }*/
/*    int nthreads = cpustr_to_cpulist(bdata(wgroup), tmp, tcount);*/
/*    config->cpulist = tmp;*/
/*    config->num_threads = nthreads;*/
/*    if (global_verbosity > DEBUGLEV_ONLY_ERROR)*/
/*    {*/
/*        bstring dstr = bformat("Workgroup '%s': ", bdata(wgroup));*/
/*        for (int j = 0; j < config->num_threads; j++)*/
/*        {*/
/*            bstring t = bformat("%d ", config->cpulist[j]);*/
/*            bconcat(dstr, t);*/
/*            bdestroy(t);*/
/*        }*/
/*        DEBUG_PRINT(DEBUGLEV_INFO, %s, bdata(dstr));*/
/*        bdestroy(dstr);*/
/*    }*/
/*    return 0;*/
/*}*/
