#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

#include <config_getopt.h>
#include <map.h>

static char baseopts_short[] = "+t:f:h";
#define NUM_BASEOPTS 3
static struct option baseopts_long[(NUM_BASEOPTS)+1] = {
    {"test", required_argument, 0, 't'},
    {"file", required_argument, 0, 'f'},
    {"help", no_argument, 0, 'h'},
    {0, 0, 0, 0}
};

#define NUM_BASETESTOPTS 3
static char basetestopts_short[] = "i:w:r:";
static struct option basetestopts_long[(NUM_BASETESTOPTS)+1] = {
    {"iterations", required_argument, 0, 'i'},
    {"workgroup", required_argument, 0, 'w'},
    {"runtime", required_argument, 0, 'r'},
    {0, 0, 0, 0}
};

static int opt_count(struct option * options)
{
    int count = 0;
    while (options[count].name != 0) { count++; }
    return count;
}

int main(int argc, char* argv[])
{
    struct option *all_opts = NULL;
    
    combine_getopt_long_opts(baseopts_long, &all_opts);
    printf("%d == %d\n", NUM_BASEOPTS, opt_count(all_opts));
    print_longopts(all_opts);
    combine_getopt_long_opts(basetestopts_long, &all_opts);
    printf("%d == %d\n", NUM_BASEOPTS+NUM_BASETESTOPTS, opt_count(all_opts));
    print_longopts(all_opts);
    
    Map_t cliopts = NULL;
    bstring sopts = bformat("%s%s",baseopts_short, basetestopts_short);
    execute_getopt(argc, argv, sopts, all_opts, &cliopts);
    destroy_smap(cliopts);

    free(all_opts);
    return 0;
}
