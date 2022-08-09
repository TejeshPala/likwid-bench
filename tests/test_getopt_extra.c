#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <error.h>
#include <bstrlib.h>
#include <bstrlib_helper.h>
#include <getopt_extra.h>
#include <map.h>

int global_verbosity = 3;

static struct option_extra options[] = {
    {"help", 'h', RETURN_TYPE_MASK(RETURN_TYPE_BOOL)},
    {"test", 't', RETURN_TYPE_MASK(RETURN_TYPE_BSTRING), required_argument},
    {"file", 'f', RETURN_TYPE_MASK(RETURN_TYPE_BSTRING), required_argument, "Filename", ARG_FLAG_MASK(ARG_FLAG_FILE)},
    {"iters", 'i', RETURN_TYPE_MASK(RETURN_TYPE_BSTRING), required_argument, "Iteration time", ARG_FLAG_MASK(ARG_FLAG_TIME)},
    {"workgroup", 'w', RETURN_TYPE_MASK(RETURN_TYPE_BSTRING)|RETURN_TYPE_MASK(RETURN_TYPE_LIST), required_argument},
    {0,0,},
};

static char* test_argv1[] = {"<exec>","--f","vla", "-t", "as", "-i", "100", "-w", "w1", "-w", "w2","-i","20"};
static int test_argc1 = 13;

static char* test_argv2[] = {"<exec>","--h"};
static int test_argc2 = 2;

static char* test_argv3[] = {"<exec>","--h", "--iters", "100ms", "--workgroup", "w1"};
static int test_argc3 = 6;

static void print_cli(int argc, char* argv[])
{
    if (argc > 0)
    {
        printf("%s", argv[0]);
    }
    for (int i = 1; i < argc; i++)
    {
        printf(", %s", argv[i]);
    }
    printf("\n");
    return;
}

int main(int argc, char* argv[])
{
    Map_t m;
    print_cli(test_argc1, test_argv1);
    getopt_long_extra(test_argc1, test_argv1, NULL, options, &m);
    destroy_smap(m);
    print_cli(test_argc2, test_argv2);
    getopt_long_extra(test_argc2, test_argv2, NULL, options, &m);
    destroy_smap(m);
    print_cli(test_argc3, test_argv3);
    getopt_long_extra(test_argc3, test_argv3, NULL, options, &m);
    destroy_smap(m);
    
    struct option_extra* options = NULL;
    printf("Empty options\n");
    print_options(options);
    printf("Add --help\n");
    add_option_params("help", 'h', RETURN_TYPE_MASK(RETURN_TYPE_BOOL), no_argument, &options, 0);
    print_options(options);
    printf("Add --test\n");
    add_option_params("test", 't', RETURN_TYPE_MASK(RETURN_TYPE_BSTRING), required_argument, &options, 0);
    print_options(options);
    printf("Add --workgroup\n");
    add_option_params("workgroup", 'w', RETURN_TYPE_MASK(RETURN_TYPE_BSTRING)|RETURN_TYPE_MASK(RETURN_TYPE_LIST), required_argument, &options, 0);
    print_options(options);
    printf("Delete --test\n");
    del_option("test", &options);
    print_options(options);
    printf("Delete --help\n");
    del_option("help", &options);
    print_options(options);
    printf("Add --help\n");
    add_option_params("help", 'h', RETURN_TYPE_MASK(RETURN_TYPE_BOOL), no_argument, &options, 0);
    print_options(options);
    free(options);
    return 0;
}
