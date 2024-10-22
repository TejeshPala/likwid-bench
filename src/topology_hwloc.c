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

#define MIN(a,b) ((a < b) ? a : b)

hwloc_topology_t topo;

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
#if defined(__x86_64) || defined(__x86_64__)
                else if (bstrncmp(bkey, &physicalString, blength(&physicalString)) == BSTR_OK && binstr(&physicalString, 0, bkey) != BSTR_ERR)
                {
                    hwloc_obj_add_info(pu, "cpu_physical", bdata(bval));
                }
#endif
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

void print_topology(hwloc_topology_t topo)
{
    printf("HWThread\tSocket\tCore\n");
    hwloc_obj_t pu = NULL;
    while ((pu = hwloc_get_next_obj_by_type(topo, HWLOC_OBJ_PU, pu)) != NULL)
    {
        hwloc_obj_t socket = hwloc_get_ancestor_obj_by_type(topo, HWLOC_OBJ_PACKAGE, pu);
        hwloc_obj_t core = hwloc_get_ancestor_obj_by_type(topo, HWLOC_OBJ_CORE, pu);
        printf("%d\t\t%d\t%d\n", pu->os_index, socket ? socket->os_index : -1, core ? core->os_index : -1);
    }
}

int cpustr_to_cpulist_physical_hwloc(bstring cpustr, int* list, int length)
{
    int idx = 0;
    struct bstrList* blist = bsplit(cpustr, ',');

    for (int i = 0; i < blist->qty; i++)
    {
        int start = 0, end = 0;
        int c = sscanf(bdata(blist->entry[i]), "%d-%d", &start, &end);
        if (c == 2)
        {
            if (start < end)
            {
                for (int j = start; j <= end && idx < length; j++)
                {
                    hwloc_obj_t obj = hwloc_get_pu_obj_by_os_index(topo, j);
                    if (obj)
                    {
                        list[idx++] = obj->os_index;
                    }
                }
            }
            else
            {
                for (int j = start; j >= end && idx < length; j--)
                {
                    hwloc_obj_t obj = hwloc_get_pu_obj_by_os_index(topo, j);
                    if (obj)
                    {
                        list[idx++] = obj->os_index;
                    }
                }
            }
        }
        else if (c == 1)
        {
            hwloc_obj_t obj = hwloc_get_pu_obj_by_os_index(topo, start);
            if (obj && idx < length)
            {
                list[idx++] = obj->os_index;
            }
        }
    }

    bstrListDestroy(blist);
    return idx;
}

int cpustr_to_cpulist_expression_hwloc(bstring cpustr, int* list, int length)
{
    int ret = 0;
    int outcount = 0;
    char domain = 'X';
    int domIdx = -1;
    int count = 0;
    int chunk = -1;
    int stride = -1;

    int c = sscanf(bdata(cpustr), "E:%c:%d:%d:%d", &domain, &count, &chunk, &stride);
    if (domain != 'N' && count == 0)
    {
        c = sscanf(bdata(cpustr), "E:%c:%d:%d:%d:%d", &domain, &domIdx, &count, &chunk, &stride);
    }

    if (domain == 'X' && count == 0)
    {
        return -EINVAL;
    }

    if ((domain == 'S' || domain == 'M' || domain == 'D') && domIdx < 0)
    {
        return -EINVAL;
    }

    hwloc_bitmap_t cpuset = hwloc_bitmap_alloc();
    if (!cpuset)
    {
        return -ENOMEM;
    }
    hwloc_obj_t obj = NULL;

    switch (domain)
    {
        case 'N':
            hwloc_bitmap_copy(cpuset, hwloc_topology_get_topology_cpuset(topo));
            break;
        case 'S':
            obj = hwloc_get_obj_by_type(topo, HWLOC_OBJ_PACKAGE, domIdx);
            if (obj)
            {
                hwloc_bitmap_copy(cpuset, obj->cpuset);
            }
            break;
        case 'M':
            obj = hwloc_get_obj_by_type(topo, HWLOC_OBJ_NUMANODE, domIdx);
            if (obj)
            {
                hwloc_bitmap_copy(cpuset, obj->cpuset);
            }
            break;
        case 'D':
            hwloc_topology_set_type_filter(topo, HWLOC_OBJ_DIE, HWLOC_TYPE_FILTER_KEEP_ALL);
            obj = hwloc_get_obj_by_depth(topo, HWLOC_OBJ_DIE, domIdx);
            if (obj)
            {
                hwloc_bitmap_copy(cpuset, obj->cpuset);
            }
            break;
    }

    ret = hwloc_bitmap_iszero(cpuset);
    if (ret)
    {
        hwloc_bitmap_free(cpuset);
        return -EINVAL;
    }

    int looplength = MIN(length, hwloc_bitmap_weight(cpuset));
    if (count > 0)
    {
        looplength = MIN(count, looplength);
    }

    unsigned int cpu_id;
    if (stride == -1 && chunk == -1)
    {
        hwloc_bitmap_foreach_begin(cpu_id, cpuset)
        {
            if (outcount >= length)
            {
                break;
            }
            list[outcount++] = cpu_id;
        } hwloc_bitmap_foreach_end();
    }
    else
    {
        int tmpcount = looplength;
        while (tmpcount > 0)
        {
            hwloc_bitmap_foreach_begin(cpu_id, cpuset)
            {
                if (tmpcount <= 0)
                {
                    break;
                }
                for (int j = 0; j < chunk && tmpcount > 0; j++)
                {
                    if (hwloc_bitmap_isset(cpuset, cpu_id + j))
                    {
                        list[outcount++] = cpu_id + j;
                        tmpcount--;
                    }
                }
                cpu_id += stride - 1;
            } hwloc_bitmap_foreach_end();
        }
    }

    hwloc_bitmap_free(cpuset);
    return outcount;
}

int cpustr_to_cpulist_logical_hwloc(bstring cpustr, int* list, int length)
{
    struct tagbstring bcolon = bsStatic(":");
    char domain = 'X';
    int domIdx = -1;
    int count = 0;
    int colon = binchr(cpustr, 0, &bcolon);
    bstring bdomain = bmidstr(cpustr, 0, colon);
    bstring blist = bmidstr(cpustr, colon + 1, blength(cpustr) - colon);

    sscanf(bdata(bdomain), "%c%d", &domain, &domIdx);

    hwloc_bitmap_t cpuset = hwloc_bitmap_alloc();
    hwloc_obj_t obj = NULL;

    switch (domain)
    {
        case 'N':
            hwloc_bitmap_copy(cpuset, hwloc_topology_get_topology_cpuset(topo));
            break;
        case 'S':
            obj = hwloc_get_obj_by_type(topo, HWLOC_OBJ_PACKAGE, domIdx);
            if (obj)
            {
                hwloc_bitmap_copy(cpuset, obj->cpuset);
            }
            break;
        case 'M':
            obj = hwloc_get_obj_by_type(topo, HWLOC_OBJ_NUMANODE, domIdx);
            if (obj)
            {
                hwloc_bitmap_copy(cpuset, obj->cpuset);
            }
            break;
        case 'D':
            hwloc_topology_set_type_filter(topo, HWLOC_OBJ_DIE, HWLOC_TYPE_FILTER_KEEP_ALL);
            obj = hwloc_get_obj_by_depth(topo, HWLOC_OBJ_DIE, domIdx);
            if (obj)
            {
                hwloc_bitmap_copy(cpuset, obj->cpuset);
            }
            break;
    }

    int ret = hwloc_bitmap_iszero(cpuset);
    if (ret)
    {
        hwloc_bitmap_free(cpuset);
        bdestroy(bdomain);
        bdestroy(blist);
        return -EINVAL;
    }

    int* idxList = NULL;
    int idxLen = 0;
    ret = resolve_list(blist, &idxLen, &idxList);
    if (ret != 0)
    {
        hwloc_bitmap_free(cpuset);
        bdestroy(bdomain);
        bdestroy(blist);
        return -EINVAL;
    }

    int outcount = 0;
    int idx = 0;
    unsigned int cpu_id;
    hwloc_bitmap_foreach_begin(cpu_id, cpuset)
    {
        if (idx >= idxLen || outcount >= length)
        {
            break;
        }
        if (idx == idxList[outcount])
        {
            list[outcount++] = cpu_id;
        }
        idx++;
    } hwloc_bitmap_foreach_end();

    hwloc_bitmap_free(cpuset);
    bdestroy(bdomain);
    bdestroy(blist);
    free(idxList);
    return outcount;
}

int cpustr_to_cpulist_hwloc(bstring cpustr, int* list, int length)
{
    DEBUG_PRINT(DEBUGLEV_DEVELOP, Using hwloc lib for cpustr);
    if (bchar(cpustr, 0) == 'E')
    {
        return cpustr_to_cpulist_expression_hwloc(cpustr, list, length);
    }
    else if (bstrchrp(cpustr, ':', 0) >= 0)
    {
        return cpustr_to_cpulist_logical_hwloc(cpustr, list, length);
    }
    else
    {
        return cpustr_to_cpulist_physical_hwloc(cpustr, list, length);
    }
    return -EINVAL;
}
