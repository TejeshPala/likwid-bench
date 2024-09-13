#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sched.h>

#include "error.h"
#include "bstrlib.h"
#include "bstrlib_helper.h"
#include "bitmap.h"
#include "path.h"

#define TOPO_MIN(a,b) ((a) < (b) ? (a) : (b))

static int _min_processor = 0;
static int _max_processor = 0;

struct tagbstring _topology_interesting_flags[] = {
#if defined(__x86_64) || defined(__x86_64__)
    bsStatic("sse"),
    bsStatic("ssse"),
    bsStatic("avx"),
    bsStatic("fma"),
    bsStatic("ht"),
    bsStatic("fp"),
#elif defined(__ARM_ARCH_8A)
    bsStatic("neon"),
    bsStatic("vfp"),
    bsStatic("asimd"),
    bsStatic("sve"),
    bsStatic("fp"),
    bsStatic("pmull"),
#endif
    bsStatic("")
};

struct tagbstring _cpu_folders[] = {
    bsStatic("present"),
    bsStatic("possible"),
    bsStatic("online"),
    bsStatic("offline"),
    bsStatic("isolated")
};

typedef struct {
    Bitmap present;
    Bitmap possible;
    Bitmap online;
    Bitmap offline;
    Bitmap isolated;
} CPULists;

static CPULists* cpu_lists = NULL;

typedef struct {
    bstring processor;
    struct {
        bstring vendor;
        bstring family;
        bstring model;
        bstring name;
        bstring stepping;
        bstring flags;
    } ProcInfo;
} CPUInfo;

static CPUInfo* cpu_info = NULL;

void destroy_cpu_lists()
{
    if (cpu_lists != NULL)
    {
        destroy_bitmap(&cpu_lists->present);
        destroy_bitmap(&cpu_lists->possible);
        destroy_bitmap(&cpu_lists->online);
        destroy_bitmap(&cpu_lists->offline);
        destroy_bitmap(&cpu_lists->isolated);
        free(cpu_lists);
        cpu_lists = NULL;
    }
}

int initialize_cpu_lists(int _max_processor)
{

    cpu_lists = (CPULists*)malloc((_max_processor + 1) * sizeof(CPULists));
    if (cpu_lists == NULL)
    {
        ERROR_PRINT(Error in allocating memory for CPULists);
        destroy_cpu_lists();
        return -ENOMEM;
    }

    memset(cpu_lists, 0, sizeof(CPULists));

    if (create_bitmap(_max_processor + 1, sizeof(BitmapDataType), &cpu_lists->present) != 0 ||
        create_bitmap(_max_processor + 1, sizeof(BitmapDataType), &cpu_lists->possible) != 0 ||
        create_bitmap(_max_processor + 1, sizeof(BitmapDataType), &cpu_lists->online) != 0 ||
        create_bitmap(_max_processor + 1, sizeof(BitmapDataType), &cpu_lists->offline) != 0 ||
        create_bitmap(_max_processor + 1, sizeof(BitmapDataType), &cpu_lists->isolated) != 0)
    {
        ERROR_PRINT(Unable to intialize bitmaps for CPULists);
        destroy_cpu_lists();
        return -ENOMEM;
    }

    return 0;
}

void initialize_cpu_info(int _max_processor)
{
    for (int p = 0; p <= _max_processor; p++)
    {
        cpu_info[p].processor = NULL;
        cpu_info[p].ProcInfo.vendor = NULL;
        cpu_info[p].ProcInfo.family = NULL;
        cpu_info[p].ProcInfo.model = NULL;
        cpu_info[p].ProcInfo.name = NULL;
        cpu_info[p].ProcInfo.stepping = NULL;
        cpu_info[p].ProcInfo.flags = NULL;
    }
}

void free_cpu_info(int _max_processor)
{
    if (cpu_info == NULL) return;

    for (int i = 0; i <= _max_processor; i ++)
    {
        if (cpu_info[i].processor != NULL)
        {
            bdestroy(cpu_info[i].processor);
        }
        if (cpu_info[i].ProcInfo.vendor != NULL)
        {
            bdestroy(cpu_info[i].ProcInfo.vendor);
        }
        if (cpu_info[i].ProcInfo.family != NULL)
        {
            bdestroy(cpu_info[i].ProcInfo.family);
        }
        if (cpu_info[i].ProcInfo.model != NULL)
        {
            bdestroy(cpu_info[i].ProcInfo.model);
        }
        if (cpu_info[i].ProcInfo.name != NULL)
        {
            bdestroy(cpu_info[i].ProcInfo.name);
        }
        if (cpu_info[i].ProcInfo.stepping != NULL)
        {
            bdestroy(cpu_info[i].ProcInfo.stepping);
        }
        if (cpu_info[i].ProcInfo.flags != NULL)
        {
            bdestroy(cpu_info[i].ProcInfo.flags);
        }
    }
    free(cpu_info);
    cpu_info = NULL;
}

typedef struct {
    int os_id;
    int smt_id;
    int core_id;
    int die_id;
    int socket_id;
    int numa_id;
    int usable;
} LikwidBenchHwthread;

static LikwidBenchHwthread* _hwthreads = NULL;
static int _num_hwthreads = 0;
static int _active_hwthreads = 0;

static int file_to_int(bstring filename, int *value)
{
    FILE* fp = NULL;
    char buffer[100];
    if (NULL != (fp = fopen (bdata(filename), "r")))
    {
        buffer[0] = '\0';
        int ret = fread(buffer, sizeof(char), 99, fp);
        if (ret >= 0)
        {
            buffer[ret] = '\0';
            int tmp = atoi(buffer);
            *value = tmp;
            fclose(fp);
            return 0;
        }
    }
    return -errno;
}

static void _print_list(int length, int* list)
{
    if (length > 0)
    {
        printf("%d", list[0]);
    }
    for (int i = 1; i < length; i++)
    {
        printf(",%d", list[i]);
    }
    printf("\n");
}

static int hwthread_online(int os_id)
{
    int online = 0;
    FILE* fp = NULL;
    //DEBUG_PRINT(DEBUGLEV_DEVELOP, Read file /sys/devices/system/cpu/online);
    if (NULL != (fp = fopen ("/sys/devices/system/cpu/online", "r")))
    {
        bstring src = bread ((bNread) fread, fp);
        struct bstrList* blist = bsplit(src, ',');
        bdestroy(src);
        for (int i = 0; i < blist->qty; i++)
        {
            int s = 0, e = 0;
            int c = sscanf(bdata(blist->entry[i]), "%d-%d", &s, &e);
            if (c == 1 && os_id == s)
            {
                online = 1;
            }
            else if (c == 2 && s <= os_id && e >= os_id)
            {
                online = 1;
            }
        }
        bstrListDestroy(blist);
        fclose(fp);
    }
    return online;
}

static int hwthread_smt_id(int os_id)
{
    int smt_id = -1;
    FILE* fp = NULL;
    bstring listfile = bformat("/sys/devices/system/cpu/cpu%d/topology/thread_siblings_list", os_id);
    //DEBUG_PRINT(DEBUGLEV_DEVELOP, Read file %s, bdata(listfile));
    if (NULL != (fp = fopen (bdata(listfile), "r")))
    {
        bstring src = bread ((bNread) fread, fp);
        struct bstrList* blist = bsplit(src, ',');
        bdestroy(src);
        for (int i = 0; i < blist->qty; i++)
        {
            int s = 0, e = 0;
            int c = sscanf(bdata(blist->entry[i]), "%d-%d", &s, &e);
            if (c == 1 && os_id == s)
            {
                smt_id = i;
                break;
            }
        }
        bstrListDestroy(blist);
        fclose(fp);
    }
    bdestroy(listfile);
    return smt_id;
}

int check_hwthreads_numa_count(int* numNumaNodes, int* maxNumaNodeId)
{
    int count = 0;
    int maxId = 0;
    DIR *dp = NULL;
    struct dirent *ep = NULL;
    //DEBUG_PRINT(DEBUGLEV_DEVELOP, Scanning directory /sys/devices/system/node);
    dp = opendir("/sys/devices/system/node");
    if (dp != NULL)
    {
        while ((ep = readdir(dp)))
        {
            int node_id = 0;
            int ret = sscanf(ep->d_name, "node%d", &node_id);
            if (ret == 1)
            {
                count++;
                if (node_id > maxId)
                {
                    maxId = node_id;
                }
            }
        }
        closedir(dp);
    }
    //DEBUG_PRINT(DEBUGLEV_DEVELOP, Available NUMA domains %d Max ID %d, count, maxId);
    *numNumaNodes = count;
    *maxNumaNodeId = maxId;
    return 0;
}

int check_hwthreads_count(int* numHWThreads, int *maxHWThreadId)
{
    DIR *dp = NULL;
    struct dirent *ep = NULL;
    int avail_hwthreads = 0;
    int max_os_id = 0;
    ////DEBUG_PRINT(DEBUGLEV_DEVELOP, Scanning directory /sys/devices/system/cpu);
    dp = opendir("/sys/devices/system/cpu");
    if (dp != NULL)
    {
        while ((ep = readdir(dp)))
        {
            int os_id = 0;
            int ret = sscanf(ep->d_name, "cpu%d", &os_id);
            if (ret == 1)
            {
                avail_hwthreads++;
                if (os_id > max_os_id)
                {
                    max_os_id = os_id;
                }
            }
        }
        closedir(dp);
    }
    //DEBUG_PRINT(DEBUGLEV_DEVELOP, Available threads %d Max ID %d, avail_hwthreads, max_os_id);
    *numHWThreads = avail_hwthreads;
    *maxHWThreadId = max_os_id;
    return 0;
}


static void print_hwthread_data(LikwidBenchHwthread* cur)
{
    //DEBUG_PRINT(DEBUGLEV_DETAIL, OSIDX %d SMT %d CORE %d DIE %d SOCKET %d NUMA %d %s,
    //            cur->os_id, cur->smt_id, cur->core_id, cur->die_id, cur->socket_id, cur->numa_id,
    //            (cur->usable == 0 ? "NOTUSABLE" : ""));
}

static int read_hwthread_data(int os_id, int maxNumaNodeId, int* smt_id, int* core_id, int* die_id, int* socket_id, int* numa_id)
{
    int core = 0;
    int die = 0;
    int socket = 0;
    int numa = 0;
    int smt = 0;
    bstring cpufolder = bformat("/sys/devices/system/cpu/cpu%d", os_id);
    const char* tmpstr = bdata(cpufolder);
    if (!access(tmpstr, R_OK|X_OK))
    {
        int ret = 0;
        int tmpid = 0;
        bstring tmpfile = bstrcpy(cpufolder);
        int tmplen = blength(tmpfile);
        bcatcstr(tmpfile, "/topology/physical_package_id");
        //DEBUG_PRINT(DEBUGLEV_DEVELOP, Read file %s, bdata(tmpfile));
        ret = file_to_int(tmpfile, &tmpid);
        if (ret == 0)
        {
            socket = tmpid;
        }
        btrunc(tmpfile, tmplen);
        bcatcstr(tmpfile, "/topology/core_id");
        //DEBUG_PRINT(DEBUGLEV_DEVELOP, Read file %s, bdata(tmpfile));
        ret = file_to_int(tmpfile, &tmpid);
        if (ret == 0)
        {
            core = tmpid;
        }
        btrunc(tmpfile, tmplen);
        bcatcstr(tmpfile, "/topology/die_id");
        //DEBUG_PRINT(DEBUGLEV_DEVELOP, Read file %s, bdata(tmpfile));
        ret = file_to_int(tmpfile, &tmpid);
        if (ret == 0)
        {
            die = tmpid;
        }
        bdestroy(tmpfile);
        for (int j = 0; j < maxNumaNodeId + 1; j++)
        {
            bstring nodefolder = bformat("/sys/devices/system/cpu/cpu%d/node%d", os_id, j);
            tmpstr = bdata(nodefolder);
            //DEBUG_PRINT(DEBUGLEV_DEVELOP, Check folder %s, tmpstr);
            if (!access(tmpstr, R_OK|X_OK))
            {
                numa = j;
                
                bdestroy(nodefolder);
                break;
            }
            bdestroy(nodefolder);
        }
        smt = hwthread_smt_id(os_id);
    }
    bdestroy(cpufolder);
    *core_id = core;
    *die_id = die;
    *socket_id = socket;
    *numa_id = numa;
    *smt_id = smt;
    return 0;
}

int check_hwthreads()
{
    if (_hwthreads == NULL)
    {
        
        DIR *dp = NULL;
        struct dirent *ep = NULL;
        cpu_set_t cpu_set;
        CPU_ZERO(&cpu_set);
        int ret = sched_getaffinity(0, sizeof(cpu_set_t), &cpu_set);
        if (ret < 0)
        {
            return -errno;
        }

        int avail_hwthreads = 0;
        int max_os_id = 0;
        check_hwthreads_count(&avail_hwthreads, &max_os_id);

        int numNumaNodes = 0;
        int maxNumaNodeId = 0;
        check_hwthreads_numa_count(&numNumaNodes, &maxNumaNodeId);
        

        int numThreads = avail_hwthreads;
        if (max_os_id + 1 > numThreads)
        {
            numThreads = max_os_id + 1;
        }
        //DEBUG_PRINT(DEBUGLEV_DEVELOP, Allocating space for %d threads, numThreads);
        int tmpCount = 0;
        int tmpCountActive = 0;
        LikwidBenchHwthread* tmpList = malloc(numThreads * sizeof(LikwidBenchHwthread));
        if (!tmpList)
        {
            printf("failed to allocate tmpList\n");
            return -ENOMEM;
        }
        memset(tmpList, 0, numThreads * sizeof(LikwidBenchHwthread));
        for (int i = 0; i < max_os_id + 1; i++)
        {
            LikwidBenchHwthread* cur = &tmpList[i];
            cur->os_id = i;
            read_hwthread_data(i, maxNumaNodeId, &cur->smt_id, &cur->core_id, &cur->die_id, &cur->socket_id, &cur->numa_id);
            if (CPU_ISSET(i, &cpu_set) && hwthread_online(i))
            {
                cur->usable = 1;
                tmpCountActive++;
            }
            tmpCount++;
        }
        //DEBUG_PRINT(DEBUGLEV_DETAIL, List filled with %d HW threads %d active, tmpCount, tmpCountActive);
        int maxCoreId = 0;
        int maxSocketId = 0;
        int maxDieId = 0;
        int maxSmtId = 0;
        for (int i = 0; i < tmpCount; i++)
        {
            LikwidBenchHwthread* cur = &tmpList[i];
            print_hwthread_data(cur);
            if (cur->core_id > maxCoreId) maxCoreId = cur->core_id;
            if (cur->socket_id > maxSocketId) maxSocketId = cur->socket_id;
            if (cur->smt_id > maxSmtId) maxSmtId = cur->smt_id;
            if (cur->die_id > maxDieId) maxDieId = cur->die_id;
        }
        _num_hwthreads = 0;
        _active_hwthreads = 0;
        _hwthreads = malloc(tmpCount * sizeof(LikwidBenchHwthread));
        if (!_hwthreads)
        {
            printf("failed to allocate _hwthreads\n");
            return -ENOMEM;
        }
        memset(_hwthreads, 0, tmpCount * sizeof(LikwidBenchHwthread));
        int idx = 0;
        for (int s = 0; s < maxSocketId + 1; s++)
        {
            for (int d = 0; d < maxDieId + 1; d++)
            {
                for (int c = 0; c < maxCoreId + 1; c++)
                {
                    for (int t = 0; t < maxSmtId + 1; t++)
                    {
                        for (int i = 0; i < tmpCount; i++)
                        {
                            LikwidBenchHwthread* test = &tmpList[i];
                            if (test && test->socket_id == s && test->die_id == d && test->core_id == c && test->smt_id == t)
                            {
                                memcpy(&_hwthreads[idx], test, sizeof(LikwidBenchHwthread));
                                _num_hwthreads++;
                                if (test->usable == 1) _active_hwthreads++;
                                //DEBUG_PRINT(DEBUGLEV_DETAIL, Move %d to %d, test->os_id, idx);
                                idx++;
                                break;
                            }
                        }
                    }
                }
            }
        }
        free(tmpList);
        //DEBUG_PRINT(DEBUGLEV_DETAIL, Final List filled with %d HW threads %d active, _num_hwthreads, _active_hwthreads);
    }
    return 0;
}

void destroy_hwthreads()
{
    if (_hwthreads)
    {
        free(_hwthreads);
        _hwthreads = NULL;
        _num_hwthreads = 0;
        _active_hwthreads = 0;
    }
}

int _hwthread_list_for_socket(int socket, int* numEntries, int** hwthreadList)
{
    int avail_hwthreads = 0;
    int ret = check_hwthreads();
    if (ret != 0)
    {
        return ret;
    }

    for (int i = 0; i < _num_hwthreads; i++)
    {
        LikwidBenchHwthread* cur = &_hwthreads[i];
        if (cur->socket_id == socket && cur->usable == 1)
        {
            avail_hwthreads++;
        }
    }
    if (avail_hwthreads == 0)
    {
        return -ENODEV;
    }
    int count = 0;
    int* list = malloc(_num_hwthreads * sizeof(int));
    if (!list)
    {
        return -ENOMEM;
    }
    memset(list, 0, _num_hwthreads * sizeof(int));

    for (int i = 0; i < _num_hwthreads; i++)
    {
        LikwidBenchHwthread* cur = &_hwthreads[i];
        if (cur->socket_id == socket && cur->usable == 1 && count < _num_hwthreads)
        {
            list[count++] = cur->os_id;
        }
    }
    *numEntries = count;
    *hwthreadList = list;
    return 0;
}

int _hwthread_list_for_numa_domain(int numa_id, int* numEntries, int** hwthreadList)
{
    int avail_hwthreads = 0;
    int ret = check_hwthreads();
    if (ret != 0)
    {
        return ret;
    }

    for (int i = 0; i < _num_hwthreads; i++)
    {
        LikwidBenchHwthread* cur = &_hwthreads[i];
        if (cur->numa_id == numa_id && cur->usable == 1)
        {
            avail_hwthreads++;
        }
    }
    if (avail_hwthreads == 0)
    {
        return -ENODEV;
    }
    int count = 0;
    int* list = malloc(_num_hwthreads * sizeof(int));
    if (!list)
    {
        return -ENOMEM;
    }
    memset(list, 0, _num_hwthreads * sizeof(int));

    for (int i = 0; i < _num_hwthreads; i++)
    {
        LikwidBenchHwthread* cur = &_hwthreads[i];
        if (cur->numa_id == numa_id && cur->usable == 1 && count < _num_hwthreads)
        {
            list[count++] = cur->os_id;
        }
    }
    *numEntries = count;
    *hwthreadList = list;
    return 0;
}

int _hwthread_list_for_cpu_die(int die_id, int* numEntries, int** hwthreadList)
{
    int avail_hwthreads = 0;
    int ret = check_hwthreads();
    if (ret != 0)
    {
        return ret;
    }

    for (int i = 0; i < _num_hwthreads; i++)
    {
        LikwidBenchHwthread* cur = &_hwthreads[i];
        if (cur->die_id == die_id && cur->usable == 1)
        {
            avail_hwthreads++;
        }
    }
    if (avail_hwthreads == 0)
    {
        return -ENODEV;
    }
    int count = 0;
    int* list = malloc(_num_hwthreads * sizeof(int));
    if (!list)
    {
        return -ENOMEM;
    }
    memset(list, 0, _num_hwthreads * sizeof(int));

    for (int i = 0; i < _num_hwthreads; i++)
    {
        LikwidBenchHwthread* cur = &_hwthreads[i];
        if (cur->die_id == die_id && cur->usable == 1 && count < _num_hwthreads)
        {
            list[count++] = cur->os_id;
        }
    }
    *numEntries = count;
    *hwthreadList = list;
    return 0;
}

LikwidBenchHwthread* getHwThread(int os_id)
{
    for (int i = 0; i < _num_hwthreads; i++)
    {
        LikwidBenchHwthread* cur = &_hwthreads[i];
        if (cur->os_id == os_id)
        {
            return cur;
        }
    }
    return NULL;
}

int _hwthread_list_sort_by_core(int length, int* hwthreadList, int** outList)
{
    int maxSocketId = 0;
    int maxCoreId = 0;
    if ((length <= 0) || (!hwthreadList) || (!outList))
    {
        return -EINVAL;
    }
    for (int i = 0; i < _num_hwthreads; i++)
    {
        LikwidBenchHwthread* cur = &_hwthreads[i];
        if (cur->usable == 1)
        {
            if (maxSocketId < cur->socket_id)
            {
                maxSocketId = cur->socket_id;
            }
            if (maxCoreId < cur->core_id)
            {
                maxCoreId = cur->core_id;
            }
        }
    }
    int count = 0;
    int* list = malloc(length * sizeof(int));
    if (!list)
    {
        return -ENOMEM;
    }
    memset(list, 0, length * sizeof(int));
    for (int s = 0; s < maxSocketId + 1; s++)
    {
        for (int c = 0; c < maxCoreId + 1; c++)
        {
            for (int i = 0; i < length; i++)
            {
                LikwidBenchHwthread* test = getHwThread(hwthreadList[i]);
                if (test)
                {
                    if (test->socket_id == s && test->core_id == c && test->usable == 1 && count < length)
                    {
                        list[count++] = hwthreadList[i];
                    }
                }
            }
        }
    }
    *outList = list;
    return 0;
}



int cpustr_to_cpulist_physical(bstring cpustr, int* list, int length)
{
    int idx = 0;
    struct bstrList* blist = bsplit(cpustr, ',');
    for (int i = 0; i < blist->qty; i++)
    {
        int s = 0, e = 0;
        int c = sscanf(bdata(blist->entry[i]), "%d-%d", &s, &e);
        if (c == 1)
        {
            if (getHwThread(s) != NULL && idx < length)
            {
                list[idx++] = s;
            }
        }
        else if (c == 2)
        {
            if (s < e)
            {
                for (int j = s; j <= e; j++)
                {
                    if (getHwThread(j) != NULL && idx < length)
                    {
                        list[idx++] = j;
                    }
                }
            }
            else
            {
                for (int j = s; j >= e; j--)
                {
                    if (getHwThread(j) != NULL && idx < length)
                    {
                        list[idx++] = j;
                    }
                }
            }
        }
    }
    bstrListDestroy(blist);
    return idx;
}

static int resolve_list(bstring bstr, int* outLength, int** outList)
{
    int len = 0;
    int *list = NULL;

    struct bstrList* blist = bsplit(bstr, ',');
    for (int i = 0; i < blist->qty; i++)
    {
        int s = 0, e = -1;
        int c = sscanf(bdata(blist->entry[i]), "%d-%d", &s, &e);
        if ((c == 1 && s >= 0) || (c == 2 && s >= 0 && e >= 0 && s == e))
        {
            int* tmp = realloc(list, (len+1) * sizeof(int));
            if (!tmp)
            {
                free(list);
                return -ENOMEM;
            }
            list = tmp;
            list[len++] = s;
        }
        else if (c == 2 && s >= 0 && e >= 0)
        {
            if (s < e)
            {
                int* tmp = realloc(list, (len+(e-s)+1) * sizeof(int));
                if (!tmp)
                {
                    free(list);
                    return -ENOMEM;
                }
                list = tmp;
                for (int j = s; j <= e; j++) list[len++] = j;
            }
            else
            {
                int* tmp = realloc(list, (len+(s-e)+1) * sizeof(int));
                if (!tmp)
                {
                    free(list);
                    return -ENOMEM;
                }
                list = tmp;
                for (int j = s; j >= e; j--) list[len++] = j;
            }
        }
    }
    bstrListDestroy(blist);
    *outLength = len;
    *outList = list;
    return 0;
}

int cpustr_to_cpulist_logical(bstring cpustr, int* list, int length)
{
    struct tagbstring bcolon = bsStatic(":");
    int c = 0;
    char domain = 'X';
    int domIdx = -1;
    int count = 0;
    int stride = 0;
    int chunk = 0;
    int ret = check_hwthreads();
    if (ret != 0)
    {
        return ret;
    }
    int colon = binchr(cpustr, 0, &bcolon);
    bstring bdomain = bmidstr(cpustr, 0, colon);
    bstring blist = bmidstr(cpustr, colon+1, blength(cpustr) - colon);
    colon = binchr(blist, 0, &bcolon);
    if (colon != BSTR_ERR)
    {
        bdestroy(bdomain);
        bdestroy(blist);
        return -EINVAL;
    }
    c = sscanf(bdata(bdomain), "%c%d", &domain, &domIdx);
    int *idxList = NULL;
    int idxLen = 0;
    ret = resolve_list(blist, &idxLen, &idxList);
    if (ret != 0)
    {
        bdestroy(bdomain);
        bdestroy(blist);
        return ret;
    }
    bdestroy(bdomain);
    bdestroy(blist);
    count = idxLen;

    if (domain == 'X' || count == 0)
    {
        return -EINVAL;
    }
    if ((domain == 'S' || domain == 'M' || domain == 'D') && domIdx < 0)
    {
        return -EINVAL;
    }

    int* tmpList = NULL;
    int tmpCount = 0;
    
    switch (domain)
    {
        case 'N':
            tmpList = malloc(_num_hwthreads * sizeof(int));
            if (!tmpList)
            {
                if (idxList)
                {
                    free(idxList);
                    idxList = NULL;
                }
                return -ENOMEM;
            }
            for (int i = 0; i < _num_hwthreads; i++)
            {
                LikwidBenchHwthread* cur = &_hwthreads[i];
                tmpList[tmpCount++] = cur->os_id;
            }
            break;
        case 'S':
            if (domIdx >= 0)
            {
                ret = _hwthread_list_for_socket(domIdx, &tmpCount, &tmpList);
                if (ret != 0)
                {
                    if (idxList)
                    {
                        free(idxList);
                        idxList = NULL;
                        idxLen = 0;
                    }
                    return ret;
                }
            }
            break;
        case 'M':
            if (domIdx >= 0)
            {
                ret = _hwthread_list_for_numa_domain(domIdx, &tmpCount, &tmpList);
                if (ret != 0)
                {
                    if (idxList)
                    {
                        free(idxList);
                        idxList = NULL;
                        idxLen = 0;
                    }
                    return ret;
                }
            }
            break;
        case 'D':
            if (domIdx >= 0)
            {
                ret = _hwthread_list_for_cpu_die(domIdx, &tmpCount, &tmpList);
                if (ret != 0)
                {
                    if (idxList)
                    {
                        free(idxList);
                        idxList = NULL;
                        idxLen = 0;
                    }
                    return ret;
                }
            }
            break;
    }
    int looplength = TOPO_MIN(length, _num_hwthreads);
    if (count > 0)
    {
        looplength = TOPO_MIN(tmpCount, looplength);
    }

    int outcount = 0;
    for (int i = 0; i < idxLen && outcount < looplength; i++)
    {
        list[outcount++] = tmpList[idxList[i]];
    }
    free(idxList);
    free(tmpList);
    return outcount;
}

int cpustr_to_cpulist_expression(bstring cpustr, int* list, int length)
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
        c = sscanf(bdata(cpustr), "E:%c%d:%d:%d:%d", &domain, &domIdx, &count, &chunk, &stride);
    }

    if (domain == 'X' || count == 0)
    {
        return -EINVAL;
    }
    if ((domain == 'S' || domain == 'M' || domain == 'D') && domIdx < 0)
    {
        return -EINVAL;
    }

    int* tmpList = NULL;
    int tmpCount = 0;
    
    switch (domain)
    {
        case 'N':
            tmpList = malloc(_num_hwthreads * sizeof(int));
            if (!tmpList)
            {
                return -ENOMEM;
            }
            for (int i = 0; i < _num_hwthreads; i++)
            {
                LikwidBenchHwthread* cur = &_hwthreads[i];
                tmpList[tmpCount++] = cur->os_id;
            }
            
            break;
        case 'S':
            if (domIdx >= 0)
            {
                ret = _hwthread_list_for_socket(domIdx, &tmpCount, &tmpList);
                if (ret != 0)
                {
                    return ret;
                }
            }
            break;
        case 'M':
            if (domIdx >= 0)
            {
                ret = _hwthread_list_for_numa_domain(domIdx, &tmpCount, &tmpList);
                if (ret != 0)
                {
                    return ret;
                }
            }
            break;
        case 'D':
            if (domIdx >= 0)
            {
                ret = _hwthread_list_for_cpu_die(domIdx, &tmpCount, &tmpList);
                if (ret != 0)
                {
                    return ret;
                }
            }
            break;
    }
    int* tmpList2;
    ret = _hwthread_list_sort_by_core(tmpCount, tmpList, &tmpList2);
    if (ret < 0)
    {
        free(tmpList);
        return ret;
    }
    free(tmpList);
    tmpList = tmpList2;
    int looplength = TOPO_MIN(length, TOPO_MIN(tmpCount, _num_hwthreads));
    if (count > 0)
    {
        looplength = TOPO_MIN(count, looplength);
    }

    if (stride == -1 && chunk == -1)
    {
        for (int i = 0; i < looplength; i++)
        {
            list[outcount++] = tmpList[i];
        }
    }
    else
    {
        int tmpcount = looplength;
        while (tmpcount > 0)
        {
            int idx = 0;
            for (idx = 0; idx < _num_hwthreads && tmpcount > 0; idx += stride)
            {
                for (int i = 0; i < chunk && tmpcount > 0; i++)
                {
                    if (idx + i < _num_hwthreads)
                    {
                        list[outcount++] = tmpList[idx+i];
                        tmpcount--;
                    }
                }
            }
        }
    }
    free(tmpList);
    return outcount;
}

int cpustr_to_cpulist(bstring cpustr, int* list, int length)
{
    if (bchar(cpustr, 0) == 'E')
    {
        return cpustr_to_cpulist_expression(cpustr, list, length);
    }
    else if (bstrchrp(cpustr, ':', 0) >= 0)
    {
        return cpustr_to_cpulist_logical(cpustr, list, length);
    }
    else
    {
        return cpustr_to_cpulist_physical(cpustr, list, length);
    }
    return -EINVAL;
}

int get_num_hw_threads()
{
    int ret = check_hwthreads();
    if (ret != 0)
    {
        return ret;
    }
    return _num_hwthreads;
}

int parse_flags(bstring flagline, struct bstrList** outlist)
{
    if (flagline == NULL || outlist == NULL) return -errno;

    struct bstrList* blist = bsplit(flagline, ' ');
    if (blist == NULL) return -errno;

    struct bstrList* btmplist = bstrListCreate();
    if (btmplist == NULL)
    {
        bstrListDestroy(blist);
        return -errno;
    }

    int i = 0;
    while (_topology_interesting_flags[i].slen > 0)
    {
        // struct tagbstring binteresting_flag = _topology_interesting_flags[i];
        for (int j = 0; j < blist->qty; j++)
        {
            int pos = binstr(blist->entry[j], 0, &(_topology_interesting_flags[i]));
            // printf("pos %d\n", pos);
            if (pos == 0) bstrListAdd(btmplist, blist->entry[j]);
            // printf("flag: %s\n", bdata(blist->entry[j]));
        }

        i++;
    }
    
    bstrListDestroy(blist);
    *outlist = bstrListCopy(btmplist);
    bstrListDestroy(btmplist);
    return (*outlist != NULL) ? 0 : -errno;
}

int check_cores(int cpu_id, bstring vendor, bstring model)
{
    struct tagbstring apple_vendor = bsStatic("0x61");
    struct tagbstring apple_model = bsStatic("0x2");
    struct tagbstring apple_ice = bsStatic("apple,icestorm");
    struct tagbstring apple_fire = bsStatic("apple,firestorm");
    bstring fname = bformat("/sys/devices/system/cpu/cpu%d/of_node/compatible", cpu_id);
    if (bstrnicmp(vendor, &apple_vendor, blength(&apple_vendor)) == 0 &&
        bstrnicmp(model, &apple_model, blength(&apple_model)) == 0)
    {
        bstring bcore = read_file(bdata(fname));
        if (blength(bcore) == 0)
        {
            ERROR_PRINT(Failed to read /sys/devices/system/cpu/cpu%d/of_node/compatible, cpu_id);
            bdestroy(bcore);
            return -errno;
        }

        if (bstrncmp(bcore, &apple_ice, blength(&apple_ice)) == 0)
        {
            printf("CPU core\t: HWThread %d is an icestorm core\n", cpu_id);
        }
        else if (bstrncmp(bcore, &apple_fire, blength(&apple_fire)) == 0)
        {
            printf("CPU core\t: HWThread %d is an firestorm core\n", cpu_id);
        }
        else
        {
            printf("CPU Core\t: HWThread %d is not found\n", cpu_id);
            bdestroy(bcore);
            return -errno;
        }

        bdestroy(bcore);
    }

    bdestroy(fname);
    return 0;
}

int update_processors()
{
    int tmp= 0;
    struct tagbstring processorString = bsStatic("processor");
    bstring src = read_file(bdata(&bcpuinfo));
    if (blength(src) == 0)
    {
        int err = errno;
        ERROR_PRINT(Error reading %s file, bdata(&bcpuinfo));
        bdestroy(src);
        return -err;
    }

    struct bstrList* blist = bsplit(src, '\n');
    if (blist == NULL)
    {
        int err = errno;
        ERROR_PRINT(Unable to split from src);
        bdestroy(src);
        return -errno;
    }


    for (int i = 0; i < blist->qty; i++)
    {
        if (bstrncmp(blist->entry[i], &processorString, blength(&processorString)) == 0)
        {
            if (sscanf(bdata(blist->entry[i]), "processor : %d", &tmp) == 1)
            {
                if (tmp == _min_processor)
                {
                    _min_processor = tmp;
                }
                else if (tmp > _max_processor)
                {
                    _max_processor = tmp;
                }

            }

        }

    }
    // printf("min: %d, max: %d\n", _min_processor, _max_processor);

    bdestroy(src);
    bstrListDestroy(blist);
    return 0;
}

void clear_processors()
{
    _min_processor = 0;
    _max_processor = 0;
}

int read_flags_line(int cpu_id, bstring* flagline)
{
    int result = 0;
    FILE *file = fopen("/proc/cpuinfo", "r");
    DEBUG_PRINT(DEBUGLEV_DEVELOP, Read file /proc/cpuinfo for interesting flags);
    if (file == NULL)
    {
        ERROR_PRINT(Failed to open /proc/cpuinfo file);
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

    struct tagbstring processorString = bsStatic("processor");
#if defined(__x86_64) || defined(__x86_64__)
    struct tagbstring vendorString = bsStatic("vendor_id");
    struct tagbstring familyString = bsStatic("cpu family");
    struct tagbstring modelString = bsStatic("model");
    struct tagbstring nameString = bsStatic("model name");
    struct tagbstring steppingString = bsStatic("stepping");
    struct tagbstring flagString = bsStatic("flags");
#elif defined(__ARM_ARCH_8A)
    struct tagbstring vendorString = bsStatic("CPU implementer");
    struct tagbstring familyString = bsStatic("CPU architecture");
    struct tagbstring modelString = bsStatic("CPU variant");
    struct tagbstring nameString = bsStatic("CPU part");
    struct tagbstring steppingString = bsStatic("CPU revision");
    struct tagbstring flagString = bsStatic("Features");
#elif defined(_ARCH_PPC)
    struct tagbstring vendorString = bsStatic("vendor_id");
    struct tagbstring familyString = bsStatic("cpu family");
    struct tagbstring modelString = bsStatic("cpu");
    struct tagbstring nameString = bsStatic("machine");
    struct tagbstring steppingString = bsStatic("revision");
#endif

    result = update_processors();
    if (result != 0)
    {
        ERROR_PRINT(Unable to update processors data from %s, bdata(&bcpuinfo));
        bdestroy(src);
        bstrListDestroy(blist);
        return result;
    }

    cpu_info = (CPUInfo*)malloc((_max_processor + 1) * sizeof(CPUInfo));
    if (cpu_info == NULL)
    {
        ERROR_PRINT(Error in allocating memory for cpu_info);
        free_cpu_info(_max_processor);
        bstrListDestroy(blist);
        bdestroy(src);
        return -errno;
    }

    initialize_cpu_info(_max_processor);
    bstring bcpu_id = bfromcstr("");
    bformata(bcpu_id, "%d", cpu_id);

    for (int p = 0; p <= _max_processor; p++)
    {
        for (int i = 0; i < blist->qty; i++)
        {
            struct bstrList* bkvpair = bsplit(blist->entry[i], ':');
            if (bkvpair->qty >= 2)
            {
                bstring bval = bstrcpy(bkvpair->entry[1]);
                btrimws(bval);
                if (bstrncmp(bkvpair->entry[0], &processorString, blength(&processorString)) == 0 && cpu_info[p].processor == NULL)
                {
                    int tmp = 0;
                    // printf("bval: %s ,", bdata(bkvpair->entry[1]));
                    if (sscanf(bdata(bkvpair->entry[1]), "%d", &tmp) == 1 && tmp == p)
                    {
                        cpu_info[p].processor = bstrcpy(bval);
                    }
                }
                else if (bstrncmp(bkvpair->entry[0], &vendorString, blength(&vendorString)) == 0 && cpu_info[p].ProcInfo.vendor == NULL)
                {
                    cpu_info[p].ProcInfo.vendor = bstrcpy(bval);
                }
                else if (bstrncmp(bkvpair->entry[0], &familyString, blength(&familyString)) == 0 && cpu_info[p].ProcInfo.family == NULL)
                {
                    cpu_info[p].ProcInfo.family = bstrcpy(bval);
                }
                else if (bstrncmp(bkvpair->entry[0], &modelString, blength(&modelString)) == 0 && cpu_info[p].ProcInfo.model == NULL)
                {
                    cpu_info[p].ProcInfo.model = bstrcpy(bval);
                }
                else if (bstrncmp(bkvpair->entry[0], &nameString, blength(&nameString)) == 0 && cpu_info[p].ProcInfo.name == NULL)
                {
                    cpu_info[p].ProcInfo.name = bstrcpy(bval);
                }
                else if (bstrncmp(bkvpair->entry[0], &steppingString, blength(&steppingString)) == 0 && cpu_info[p].ProcInfo.stepping == NULL)
                {
                    cpu_info[p].ProcInfo.stepping = bstrcpy(bval);
                }

#if defined(__x86_64) || defined(__x86_64__)
                else if (bstrncmp(bkvpair->entry[0], &flagString, blength(&flagString)) == 0 && cpu_info[p].ProcInfo.flags == NULL)
#elif defined(__ARM_ARCH_8A)
                else if (bstrncmp(bkvpair->entry[0], &flagString, blength(&flagString)) == 0 && cpu_info[p].ProcInfo.flags == NULL)
#endif
                {
                    cpu_info[p].ProcInfo.flags = bstrcpy(bval);
                }

                bdestroy(bval);
            }

            bstrListDestroy(bkvpair);
        }

    }

    for (int i = 0; i <= _max_processor; i++)
    {
        if (i == cpu_id && cpu_info[i].processor != NULL)
        {
            if (cpu_info[i].ProcInfo.flags != NULL)
            {
                bconcat(*flagline, cpu_info[i].ProcInfo.flags);
            }

            if (DEBUGLEV_DEVELOP)
            {
                printf("CPU processor\t: %s\n", bdata(cpu_info[i].processor));
                printf("CPU vendor\t: %s\n", bdata(cpu_info[i].ProcInfo.vendor));
                printf("CPU family\t: %s\n", bdata(cpu_info[i].ProcInfo.family));
                printf("CPU model\t: %s\n", bdata(cpu_info[i].ProcInfo.model));
                printf("CPU name\t: %s\n", bdata(cpu_info[i].ProcInfo.name));
                printf("CPU stepping\t: %s\n", bdata(cpu_info[i].ProcInfo.stepping));
                // printf("CPU flags\t: %s\n", bdata(cpu_info[i].ProcInfo.flags));
                result = check_cores(cpu_id, cpu_info[i].ProcInfo.vendor, cpu_info[i].ProcInfo.model);
                if (result != 0)
                {
                    free_cpu_info(_max_processor);
                    bdestroy(src);
                    bdestroy(bcpu_id);
                    bstrListDestroy(blist);
                    return -errno;
                }
            }

        }

    }

    free_cpu_info(_max_processor);
    bdestroy(src);
    bdestroy(bcpu_id);
    bstrListDestroy(blist);
    clear_processors();
    return 0;
}

int get_feature_flags(int cpu_id, struct bstrList** outlist)
{
    int result = 0;
    if (outlist == NULL) return -errno;

    bstring flagline = bfromcstr("");
    result = read_flags_line(cpu_id, &flagline);
    // printf("content: %s\n", bdata(flagline));
    if (result != 0)
    {
        ERROR_PRINT(Failed to read cpu flags from /proc/cpuinfo file);
        bdestroy(flagline);
        return -errno;
    }

    bstrListDestroy(*outlist);
    struct bstrList* blist = NULL;
    result = parse_flags(flagline, &blist);
    bdestroy(flagline);
    if (result != 0)
    {
        bstrListDestroy(blist);
        ERROR_PRINT(Unable to parse the flags);
        return -errno;
    }

    *outlist = bstrListCopy(blist);
    bstrListDestroy(blist);
    DEBUG_PRINT(DEBUGLEV_DEVELOP, Available flags are );
    if (DEBUGLEV_DEVELOP) bstrListPrint(*outlist);

    return 0;
}

Bitmap* get_bitmap(const bstring fname)
{
    if (biseq(fname, &_cpu_folders[0])) return &cpu_lists->present;
    if (biseq(fname, &_cpu_folders[1])) return &cpu_lists->possible;
    if (biseq(fname, &_cpu_folders[2])) return &cpu_lists->online;
    if (biseq(fname, &_cpu_folders[3])) return &cpu_lists->offline;
    if (biseq(fname, &_cpu_folders[4])) return &cpu_lists->isolated;
}

int read_cpu_cores(bstring fname, Bitmap* bm)
{
    int start, end = 0;
    int res = 0;
    bstring src = read_file(bdata(fname));
    if (src == NULL || blength(src) == 0)
    {
        int err = errno;
        ERROR_PRINT(Failed to read %s, bdata(fname));
        bdestroy(src);
        return -err;
    }

    struct bstrList* blist = bsplit(src, ',');
    bdestroy(src);
    if (blist == NULL)
    {
        ERROR_PRINT(Failed to split src);
        bstrListDestroy(blist);
        return -errno;
    }


    for (int i = 0; i < blist->qty; i++)
    {
        int c = sscanf(bdata(blist->entry[i]), "%d-%d", &start, &end);
        if (c == 1)
        {
            // printf("setting bits at index %d\n", start);
            res = set_bit(bm, start);
            if (res != 0)
            {
                printf("set bit Failed\n");
                return res;
            }
            // printf("start: %d\n", start);
        }
        else if (c == 2)
        {
            // printf("setting bits from %d-%d\n", start, end);
            for (int b = start; b <= end; ++b)
            {
                res = set_bit(bm, b);
                if (res != 0)
                {
                    printf("set bits Failed\n");
                    return res;
                }
                // printf("fname: %s,start-end: %d-%d\n", bdata(fname), start, end);
            }
        }
    }

    if (DEBUGLEV_DEVELOP)
    {
        DEBUG_PRINT(DEBUGLEV_DEVELOP, Bits set for %s, bdata(fname));
        print_set_bits(bm);
    }
    bstrListDestroy(blist);
    return 0;
}

int parse_cpu_folders()
{
    int result = 0;
    result = update_processors();
    if (result != 0)
    {
        ERROR_PRINT(Unable to update processors data from %s, bdata(&bcpuinfo));
        clear_processors();
        return result;
    }

    result = initialize_cpu_lists(_max_processor);
    if (result != 0)
    {
        ERROR_PRINT(Failed to intilialie CPULists);
        clear_processors();
        destroy_cpu_lists();
        return result;
    }

    for (int i = 0; i < sizeof(_cpu_folders) / sizeof(_cpu_folders[0]); ++i)
    {
        bstring bfile = bformat("%s/%s", bdata(&cpu_dir), _cpu_folders[i].data);
        Bitmap* bm = get_bitmap(&_cpu_folders[i]);
        result = read_cpu_cores(bfile, bm);
        if (result != 0)
        {
            ERROR_PRINT(Failed to read %s, bdata(bfile));
            bdestroy(bfile);
            return result;
        }

        bdestroy(bfile);
    }

    clear_processors();
    destroy_cpu_lists();
    return 0;
}
