#include <errno.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "error.h"
#include "bstrlib.h"
#include "bstrlib_helper.h"
#include "topology.h"
#include "test_types.h"

#ifdef LIKWID_USE_HWLOC
#include "hwloc.h"
#include "topology_hwloc.h"
#endif

int collect_cpuinfo(hwloc_topology_t topo, RuntimeConfig* runcfg)
{
    FILE *file = fopen("/proc/cpuinfo", "r");
    if (file == NULL)
    {
        ERROR_PRINT(Failed to open /proc/cpuinfo: %s, strerror(errno));
        return -errno;
    }

    bstring src = bread ((bNread) fread, file);
    fclose(file);
    if (src == NULL)
    {
        ERROR_PRINT(Unable to read file using bread);
        return -errno;
    }

    struct bstrList* blist = bsplit(src, '\n');
    if (blist == NULL)
    {
        bdestroy(src);
        ERROR_PRINT(Unable to split from src);
        return -errno;
    }

    struct tagbstring btrue = bsStatic("true");
    struct tagbstring bfalse = bsStatic("false");
    struct tagbstring bspace = bsStatic(" ");
    struct tagbstring bunderscroll = bsStatic("_");
    struct tagbstring bempty = bsStatic("");
    struct tagbstring processorString = bsStatic("processor");
#if defined(__x86_64) || defined(__x86_64__)
    struct tagbstring vendorString = bsStatic("vendor_id");
    struct tagbstring familyString = bsStatic("cpu family");
    struct tagbstring modelString = bsStatic("model");
    struct tagbstring nameString = bsStatic("model name");
    struct tagbstring steppingString = bsStatic("stepping");
    struct tagbstring physicalString = bsStatic("physical id");
    struct tagbstring flagString = bsStatic("flags");
#elif defined(__ARM_ARCH_8A) || defined(__aarch64__) || defined(__arm__)
    struct tagbstring vendorString = bsStatic("CPU implementer");
    struct tagbstring familyString = bsStatic("CPU architecture");
    struct tagbstring modelString = bsStatic("CPU variant");
    struct tagbstring nameString = bsStatic("CPU part");
    struct tagbstring steppingString = bsStatic("CPU revision");
    struct tagbstring flagString = bsStatic("Features");
#elif defined(_ARCH_PPC) || defined(__powerpc) || defined(__ppc__) || defined(__PPC__)
    struct tagbstring vendorString = bsStatic("vendor_id");
    struct tagbstring familyString = bsStatic("cpu family");
    struct tagbstring modelString = bsStatic("cpu");
    struct tagbstring nameString = bsStatic("machine");
    struct tagbstring steppingString = bsStatic("revision");
#endif

    hwloc_obj_t pu = NULL;

    for (int i = 0; i < blist->qty; i++)
    {
        struct bstrList* bkvpair = bsplit(blist->entry[i], ':');
        if (bkvpair->qty >= 2)
        {
            bstring bkey = bstrcpy(bkvpair->entry[0]);
            bstring bval = bstrcpy(bkvpair->entry[1]);
            btrimws(bkey);
            btrimws(bval);
            int tmp = -1;
            if (bstrncmp(bkey, &processorString, blength(&processorString)) == BSTR_OK)
            {
                if (sscanf(bdata(bval), "%d", &tmp) == 1)
                {
                    pu = hwloc_get_obj_by_type(topo, HWLOC_OBJ_PU, tmp);
                }
            }
            else if (pu)
            {
                if (bstrncmp(bkey, &nameString, blength(&nameString)) == BSTR_OK && binstr(&nameString, 0, bkey) != BSTR_ERR)
                {
                    hwloc_obj_add_info(pu, "cpu_name", bdata(bval));
                }
                else if (bstrncmp(bkey, &vendorString, blength(&vendorString)) == BSTR_OK && binstr(&vendorString, 0, bkey) != BSTR_ERR)
                {
                    hwloc_obj_add_info(pu, "cpu_vendor", bdata(bval));
                }
                else if (bstrncmp(bkey, &modelString, blength(&modelString)) == BSTR_OK && binstr(&modelString, 0, bkey) != BSTR_ERR)
                {
                    hwloc_obj_add_info(pu, "cpu_model", bdata(bval));
                }
                else if (bstrncmp(bkey, &familyString, blength(&familyString)) == BSTR_OK && binstr(&familyString, 0, bkey) != BSTR_ERR)
                {
                    hwloc_obj_add_info(pu, "cpu_family", bdata(bval));
                }
                else if (bstrncmp(bkey, &steppingString, blength(&steppingString)) == BSTR_OK && binstr(&steppingString, 0, bkey) != BSTR_ERR)
                {
                    hwloc_obj_add_info(pu, "cpu_stepping", bdata(bval));
                }
                else if (bstrncmp(bkey, &physicalString, blength(&physicalString)) == BSTR_OK && binstr(&physicalString, 0, bkey) != BSTR_ERR)
                {
                    hwloc_obj_add_info(pu, "cpu_physical", bdata(bval));
                }
                else if (bstrncmp(bkey, &flagString, blength(&flagString)) == BSTR_OK && binstr(&flagString, 0, bkey) != BSTR_ERR)
                {
                    struct bstrList* flist = NULL;
                    parse_flags(bval, &flist);
                    bstring bflags = bjoin(flist, &bspace);
                    hwloc_obj_add_info(pu, "cpu_flags", bdata(bflags));
                    bdestroy(bflags);
                    if (runcfg->tcfg->flags->qty > 0 && flist->qty > 0)
                    {
                        for (int i = 0; i < runcfg->tcfg->flags->qty; i++)
                        {
                            bstring btmp = bstrcpy(runcfg->tcfg->flags->entry[i]);
                            btrimws(btmp);
                            bfindreplace(btmp, &bunderscroll, &bempty, 0);
                            btoupper(btmp);
                            for (int j = 0; j < flist->qty; j++)
                            {
                                btrimws(flist->entry[j]);
                                if (bstrnicmp(btmp, flist->entry[j], blength(btmp)) == BSTR_OK && blength(btmp) > 0 && binstr(btmp, 0, flist->entry[j]) != BSTR_ERR)
                                {
                                    hwloc_obj_add_info(pu, "cpu_flag_found", bdata(&btrue));
                                }
                                else if (blength(btmp) == 0)
                                {
                                    hwloc_obj_add_info(pu, "cpu_flag_found", bdata(&bfalse));
                                }
                            }
                            bdestroy(btmp);
                        }
                    }
                    bstrListDestroy(flist);
                }
            }
            bdestroy(bkey);
            bdestroy(bval);
        }
        bstrListDestroy(bkvpair);
    }

    bstrListDestroy(blist);
    bdestroy(src);
    return 0;
}

void print_cpuinfo(hwloc_topology_t topo)
{
    const char* infos[] = {
        "cpu_vendor",
        "cpu_family",
        "cpu_model",
        "cpu_name",
        "cpu_stepping",
        "cpu_physical",
        "cpu_flags",
        "cpu_flag_found"
    };
    int infos_count = 8;

    for (int i = 0; i < hwloc_get_nbobjs_by_type(topo, HWLOC_OBJ_PU); i++)
    {
        hwloc_obj_t pu = hwloc_get_obj_by_type(topo, HWLOC_OBJ_PU, i);
        printf("CPU %d:\n", i);
        for (int n = 0; n < infos_count; n++)
        {
            const char* tmp = hwloc_obj_get_info_by_name(pu, infos[n]);
            if (tmp)
            {
                printf("\t%s\t: %s\n", infos[n], tmp);
            }
        }
    }
    printf("\n");
}
