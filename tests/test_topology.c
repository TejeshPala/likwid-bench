#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "error.h"
#include "bstrlib.h"
#include "topology.h"

int global_verbosity = DEBUGLEV_DEVELOP;

typedef struct topology_config {
    struct tagbstring cpustr;
    int expected;
} TopoConfig;

static TopoConfig testconfigs[] = {
    {bsStatic("M0:0-4"), 5},
    {bsStatic("N:0,4"), 2},
    {bsStatic("E:S0:8"), 8},
    {bsStatic("0,1,2-3"), 4},
    {bsStatic("E:S0:3:1:2"), 3},
    {bsStatic("M10000:0-4"), -ENODEV},
    {bsStatic("N:-1"), -EINVAL},
    {bsStatic("3-0"), 4},
    {bsStatic(""), 0}, // marks end of list
};

void print_list(int length, int* list)
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

int main(int argc, char* argv[])
{
    int length = sysconf(_SC_NPROCESSORS_CONF);
    int* list = malloc(length * sizeof(int));
    if (!list)
    {
        return -ENOMEM;
    }
    
    int idx = 0;
    int failed = 0;
    int should_fail = 0;
    int success = 0;
    int unknown = 0;
    while (blength(&testconfigs[idx].cpustr) > 0)
    {
        int newlen = 0;
        newlen = cpustr_to_cpulist(&testconfigs[idx].cpustr, list, length);
        if (newlen < 0)
        {
            if (newlen == testconfigs[idx].expected)
            {
                should_fail++;
            }
            else
            {
                failed++;
            }
        }
        else if (newlen == testconfigs[idx].expected)
        {
            success++;
            print_list(newlen, list);
        }
        else
        {
            printf("Unknown error for %s with expected result %d but got %d\n", bdata(&testconfigs[idx].cpustr), testconfigs[idx].expected, newlen);
            unknown++;
        }
        idx++;
    }
    printf("Success %d Fail %d ShouldFail %d Unknown %d\n", success, failed, should_fail, unknown);
    free(list);
    destroy_hwthreads();
    return 0;
}
