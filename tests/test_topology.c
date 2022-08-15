#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "error.h"
#include "bstrlib.h"
#include "topology.h"

int global_verbosity = DEBUGLEV_DEVELOP;





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
    
    int* list = malloc(100 * sizeof(int));
    if (!list)
    {
        return -ENOMEM;
    }
    int length = 100;
    int newlen = 0;
    struct tagbstring cpustr1 = bsStatic("M0:0-4");
    struct tagbstring cpustr2 = bsStatic("N:0,4");
    struct tagbstring cpustr3 = bsStatic("E:S0:8");
    struct tagbstring cpustr4 = bsStatic("0,1,2-3");
    struct tagbstring cpustr5 = bsStatic("E:S0:3:1:2");
    newlen = cpustr_to_cpulist(&cpustr1, list, length);
    print_list(newlen, list);
    newlen = cpustr_to_cpulist(&cpustr2, list, length);
    print_list(newlen, list);
    newlen = cpustr_to_cpulist(&cpustr3, list, length);
    print_list(newlen, list);
    newlen = cpustr_to_cpulist(&cpustr4, list, length);
    print_list(newlen, list);
    newlen = cpustr_to_cpulist(&cpustr5, list, length);
    print_list(newlen, list);
    free(list);
    destroy_hwthreads();
    return 0;
}
