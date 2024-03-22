#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>


#include <sched.h>

#include "error.h"
#include "bstrlib.h"

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
    int count = 0;
    int* list = malloc(avail_hwthreads * sizeof(int));
    if (!list)
    {
        return -ENOMEM;
    }
    memset(list, 0, avail_hwthreads * sizeof(int));

    for (int i = 0; i < _num_hwthreads; i++)
    {
        LikwidBenchHwthread* cur = &_hwthreads[i];
        if (cur->socket_id == socket && cur->usable == 1)
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
    int count = 0;
    int* list = malloc(avail_hwthreads * sizeof(int));
    if (!list)
    {
        return -ENOMEM;
    }
    memset(list, 0, avail_hwthreads * sizeof(int));

    for (int i = 0; i < _num_hwthreads; i++)
    {
        LikwidBenchHwthread* cur = &_hwthreads[i];
        if (cur->numa_id == numa_id && cur->usable == 1)
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
    int count = 0;
    int* list = malloc(avail_hwthreads * sizeof(int));
    if (!list)
    {
        return -ENOMEM;
    }
    memset(list, 0, avail_hwthreads * sizeof(int));

    for (int i = 0; i < _num_hwthreads; i++)
    {
        LikwidBenchHwthread* cur = &_hwthreads[i];
        if (cur->die_id == die_id && cur->usable == 1)
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
                    if (test->socket_id == s && test->core_id == c && test->usable == 1)
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
            for (int j = s; j <= e; j++)
            {
                if (getHwThread(j) != NULL && idx < length)
                {
                    list[idx++] = j;
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
    int looplength = (length < _num_hwthreads ? length : _num_hwthreads);
    if (count > 0)
    {
        looplength = (count < looplength ? count : looplength);
    }

    int outcount = 0;
    for (int i = 0; i < idxLen; i++)
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

    int looplength = (length < _num_hwthreads ? length : _num_hwthreads);
    if (count > 0)
    {
        looplength = (count < looplength ? count : looplength);
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
