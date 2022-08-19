#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/utsname.h>

#include "error.h"
#include "bstrlib.h"
#include "bstrlib_helper.h"



#include "test_types.h"
#include "getopt_extra.h"
#include "read_yaml_ptt.h"
#include "cli_parser.h"
#include "workgroups.h"
#include "ptt2asm.h"
#include "allocator.h"

#ifndef global_verbosity
int global_verbosity = DEBUGLEV_DEVELOP;
#endif


static bstring get_architecture()
{
    struct utsname buffer;
    int ret = uname(&buffer);
    if (ret < 0)
    {
        return bfromcstr("NONE");
    }
    return bfromcstr(buffer.machine);
}

int allocate_runtime_config(RuntimeConfig** config)
{
    RuntimeConfig* runcfg = malloc(sizeof(RuntimeConfig));
    if (!runcfg)
    {
        ERROR_PRINT(Error allocating space for runtime configuration)
        return -ENOMEM;
    }
    memset(runcfg, 0, sizeof(RuntimeConfig));
    runcfg->wgroups = NULL;
    runcfg->tcfg = NULL;
    runcfg->codelines = NULL;
    runcfg->streams = NULL;
    runcfg->testname = bfromcstr("");
    runcfg->pttfile = bfromcstr("");
    runcfg->tmpfolder = bfromcstr("");
    *config = runcfg;
    return 0;
}

void free_runtime_config(RuntimeConfig* runcfg)
{
    if (runcfg)
    {
        DEBUG_PRINT(DEBUGLEV_DEVELOP, Destroy pttfile in RuntimeConfig);
        bdestroy(runcfg->pttfile);
        DEBUG_PRINT(DEBUGLEV_DEVELOP, Destroy testname in RuntimeConfig);
        bdestroy(runcfg->testname);
        DEBUG_PRINT(DEBUGLEV_DEVELOP, Destroy tmpfolder in RuntimeConfig);
        bdestroy(runcfg->tmpfolder);
        if (runcfg->wgroups)
        {
            DEBUG_PRINT(DEBUGLEV_DEVELOP, Destroy workgroups in RuntimeConfig);
            for (int i = 0; i < runcfg->num_wgroups; i++)
            {
                bdestroy(runcfg->wgroups[i].str);
                if (runcfg->wgroups[i].threads)
                {
                    free(runcfg->wgroups[i].threads);
                }
                if (runcfg->wgroups[i].cpulist)
                {
                    free(runcfg->wgroups[i].cpulist);
                    runcfg->wgroups[i].cpulist = NULL;
                    runcfg->wgroups[i].num_threads = 0;
                }
            }
            free(runcfg->wgroups);
            runcfg->wgroups = NULL;
            runcfg->num_wgroups = 0;
        }
        if (runcfg->params)
        {
            DEBUG_PRINT(DEBUGLEV_DEVELOP, Destroy parameter in RuntimeConfig);
            for (int i = 0; i < runcfg->num_params; i++)
            {
                bdestroy(runcfg->params[i].name);
                if (runcfg->params[i].type == RETURN_TYPE_BSTRING)
                {
                    bdestroy(runcfg->params[i].value.bstrvalue);
                }
                else if (runcfg->params[i].type == RETURN_TYPE_STRING)
                {
                    free(runcfg->params[i].value.strvalue);
                }
            }
            free(runcfg->params);
            runcfg->params = NULL;
            runcfg->num_params = 0;
        }
        if (runcfg->tcfg)
        {
            DEBUG_PRINT(DEBUGLEV_DEVELOP, Destroy TestConfig in RuntimeConfig);
            close_yaml_ptt(runcfg->tcfg);
            runcfg->tcfg = NULL;
        }
        if (runcfg->codelines)
        {
            bstrListDestroy(runcfg->codelines);
            runcfg->codelines = NULL;
        }
        if (runcfg->streams)
        {
            for (int i = 0; i < runcfg->num_streams; i++)
            {
                free(runcfg->streams[i].ptr);
            }
            free(runcfg->streams);
            runcfg->streams = NULL;
            runcfg->num_streams = 0;
        }
        free(runcfg);
    }
}

int main(int argc, char** argv)
{
    int c = 0, err = 0, print_help = 0;
    int option_index = -1;
    //BenchmarkConfig *cfg = NULL;
    RuntimeConfig* runcfg = NULL;
    Map_t useropts = NULL;
    int got_testcase = 0;
/*    bstring yamlfile = bfromcstr("");*/
    //bstring bbaseopts = bfromcstr(baseopts_short);
    //bstring bbasetestopts = bfromcstr(basetestopts_short);
    
/*    bstring bccflags = bfromcstr("-fPIC -shared");*/

    bstring arch = get_architecture();
#ifdef LIKWIDBENCH_KERNEL_FOLDER
    bstring kernelfolder = bformat("%s/%s/", TOSTRING(LIKWIDBENCH_KERNEL_FOLDER), bdata(arch));
#else
    bstring kernelfolder = bformat("/tmp/likwid-bench-%d/%s/", getpid(), bdata(arch));
#endif
    bdestroy(arch);
    int (*ownaccess)(const char*, int) = access;

    if (argc ==  1)
    {
        usage_print_header();
        usage_print_baseopts(1);
        goto main_out;
    }

    /*
     * Allocate configuration structure for this tun
     */
    err = allocate_runtime_config(&runcfg);
    if (err != 0)
    {
        goto main_out;
    }
    bconcat(runcfg->tmpfolder, kernelfolder);
    bdestroy(kernelfolder);

    /*
     * Prepare short and long options for getopt
     */
    err = parse_baseopts(argc, argv, runcfg);
    if (err < 0)
    {
        usage_print_header();
        usage_print_baseopts(1);
        usage_print_basetestopts();
        goto main_out;
    }
    if (runcfg->help && (blength(runcfg->testname) + blength(runcfg->pttfile)) == 0)
    {
        usage_print_header();
        usage_print_baseopts(1);
        usage_print_basetestopts();
        goto main_out;
    }
    if (runcfg->verbosity > 0)
    {
        global_verbosity = runcfg->verbosity;
    }
    if (blength(runcfg->testname) > 0)
    {
        bdestroy(runcfg->pttfile);
        runcfg->pttfile = bformat("%s/%s.yaml", bdata(runcfg->tmpfolder), bdata(runcfg->testname));
        got_testcase = 1;
    }
    else if (blength(runcfg->pttfile) > 0)
    {
        struct tagbstring bdot = bsStatic(".");
        bdestroy(runcfg->testname);
        runcfg->testname = bformat("%s", basename(bdata(runcfg->pttfile)));
        int dot = binstrr(runcfg->testname, blength(runcfg->testname)-1, &bdot);
        if (dot != BSTR_ERR && dot >= 0)
        {
            btrunc(runcfg->testname, dot);
        }
        got_testcase = 1;
    }
    else
    {
        usage_print_header();
        usage_print_baseopts(1);
        usage_print_basetestopts();
        goto main_out;
    }

    err = read_yaml_ptt(bdata(runcfg->pttfile), &runcfg->tcfg);
    if (err < 0)
    {
        fprintf(stderr, "Error reading %s\n", bdata(runcfg->pttfile));
        goto main_out;
    }

    if (runcfg->help && got_testcase)
    {
        usage_print_header();
        usage_print_baseopts(0);
        usage_print_basetestopts();
        print_test_usage(runcfg->tcfg);
        goto main_out;
    }
    err = parse_testopts(argc, argv, runcfg->tcfg, runcfg);
    if (err < 0)
    {
        printf("Parsing of test CLI options failed\n");
        goto main_out;
    }

    /*
     * Check if all required benchmark parameters are available
     */
    int miss = check_required_params(runcfg->tcfg, runcfg);
    if (miss > 0)
    {
        ERROR_PRINT(Required parameters missing);
        goto main_out;
    }

    /*
     * Analyse workgroups
     */
    err = resolve_workgroups(runcfg->num_wgroups, runcfg->wgroups);
    if (err < 0)
    {
        ERROR_PRINT(Error resolving workgroups);
        goto main_out;
    }
    for (int i = 0; i < runcfg->num_wgroups; i++)
    {
        print_workgroup(&runcfg->wgroups[i]);
    }

    /*
     * Evaluate variables, constants, ... for remaining operations
     * There should be now all values available
     */

    /*
     * Generate assembly
     */
    runcfg->codelines = bstrListCreate();
    err = generate_code(runcfg->tcfg, runcfg->codelines);
    if (err < 0)
    {
        ERROR_PRINT(Error generating code);
        goto main_out;
    }
    for (int i = 0; i < runcfg->codelines->qty; i++)
    {
        DEBUG_PRINT(global_verbosity, "CODE: %s\n", bdata(runcfg->codelines->entry[i]));
    }

    /*
     * Allocate arrays
     */
     err = allocate_streams(runcfg);
     if (err < 0)
     {
        ERROR_PRINT(Error allocating streams);
        goto main_out;
     }

    /*
     * Start threads
     */

    /*
     * Init arrays
     */

    /*
     * Prepare thread runtime info
     */

    /*
     * Run benchmark
     */

    /*
     * Free arrays
     */

    /*
     * Destroy threads
     */

    /*
     * Calculate metrics
     */

     /*
     * Print everything
     */


main_out:
    DEBUG_PRINT(DEBUGLEV_DEVELOP, MAIN_OUT);
    free_runtime_config(runcfg);

    return 0;
}
