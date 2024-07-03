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
#include "read_yaml_ptt.h"
#include "cli_parser.h"
#include "workgroups.h"
#include "ptt2asm.h"
#include "allocator.h"
#include "results.h"
#include "topology.h"

#ifndef global_verbosity
int global_verbosity = DEBUGLEV_DEVELOP;
#endif

static struct tagbstring app_title = bsStatic("likwid-bench");


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
    runcfg->params = NULL;
    runcfg->global_results = NULL;
    runcfg->testname = bfromcstr("");
    runcfg->pttfile = bfromcstr("");
    runcfg->tmpfolder = bfromcstr("");
    runcfg->kernelfolder = bfromcstr("");
    runcfg->arraysize = bfromcstr("");
    runcfg->iterations = -1;
    runcfg->runtime = -1.0;
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
        DEBUG_PRINT(DEBUGLEV_DEVELOP, Destroy kernelfolder in RuntimeConfig);
        bdestroy(runcfg->kernelfolder);
	DEBUG_PRINT(DEBUGLEV_DEVELOP, Destroy arraysize in RuntimeConfig);
        bdestroy(runcfg->arraysize);
        if (runcfg->wgroups)
        {
            DEBUG_PRINT(DEBUGLEV_DEVELOP, Destroy workgroups in RuntimeConfig);
            for (int i = 0; i < runcfg->num_wgroups; i++)
            {
                bdestroy(runcfg->wgroups[i].str);
                if (runcfg->wgroups[i].results)
                {
                    for (int j = 0; j < runcfg->wgroups[i].num_threads; j++)
                    {
                        destroy_result(&runcfg->wgroups[i].results[j]);
                    }
                    destroy_result(runcfg->wgroups[i].group_results);
                    free(runcfg->wgroups[i].results);
                    runcfg->wgroups[i].results = NULL;
                }
                if (runcfg->wgroups[i].threads)
                {
                    free(runcfg->wgroups[i].threads);
                    runcfg->wgroups[i].threads = NULL;
                }
                if (runcfg->wgroups[i].hwthreads)
                {
                    free(runcfg->wgroups[i].hwthreads);
                    runcfg->wgroups[i].hwthreads = NULL;
                    runcfg->wgroups[i].num_threads = 0;
                }
                if (runcfg->wgroups[i].streams)
                {
                    for (int w = 0; w < runcfg->wgroups[i].num_streams; w++)
                    {
                        if (runcfg->wgroups[i].streams[w].ptr)
                        {
                            free(runcfg->wgroups[i].streams[w].ptr);
                        }
                        bdestroy(runcfg->wgroups[i].streams[w].name);
                    }
                    free(runcfg->wgroups[i].streams);
                    runcfg->wgroups[i].streams = NULL;
                    runcfg->wgroups[i].num_streams = 0;
                }
                if (runcfg->wgroups[i].params)
                {
                    for (int j = 0; j < runcfg->wgroups[i].num_params; j++)
                    {
                        bdestroy(runcfg->wgroups[i].params[j].name);
                        bdestroy(runcfg->wgroups[i].params[j].value);
                    }
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
                if (runcfg->params[i].value) bdestroy(runcfg->params[i].value);
                else if (runcfg->params[i].values) bstrListDestroy(runcfg->params[i].values);
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

        if (runcfg->global_results->variables)
        {
            destroy_result(runcfg->global_results);
            free(runcfg->global_results);
            runcfg->global_results = NULL;
        }
        free(runcfg);
    }
}

int main(int argc, char** argv)
{
    int c = 0, err = 0, print_help = 0;
    int option_index = -1;
    RuntimeConfig* runcfg = NULL;
    Map_t useropts = NULL;
    struct bstrList* args = NULL;
    int got_testcase = 0;
    CliOptions baseopts = {
        .num_options = 0,
        .options = NULL,
        .title = &app_title,
    };
    CliOptions testopts = {
        .num_options = 0,
        .options = NULL,
    };
    addConstCliOptions(&baseopts, &basecliopts);
/*    bstring bccflags = bfromcstr("-fPIC -shared");*/

    bstring arch = get_architecture();
#ifdef LIKWIDBENCH_KERNEL_FOLDER
    bstring kernelfolder = bformat("%s/%s/", TOSTRING(LIKWIDBENCH_KERNEL_FOLDER), bdata(arch));
#else
    bstring kernelfolder = bformat("./");
#endif
    bstring tmpfolder = bformat("/tmp/likwid-bench-%d/%s/", getpid(), bdata(arch));
    bdestroy(arch);
    int (*ownaccess)(const char*, int) = access;

    if (argc ==  1)
    {
        printCliOptions(&baseopts);
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
    bconcat(runcfg->kernelfolder, kernelfolder);
    bconcat(runcfg->tmpfolder, tmpfolder);

    /*
     * Get command line arguments
     */
    args = bstrListCreate();
    for (int i = 1; i < argc; i++)
    {
        bstrListAddChar(args, argv[i]);
    }

    parseCliOptions(args, &baseopts);
    err = assignBaseCliOptions(&baseopts, runcfg);

    //err = parse_baseopts(argc, argv, runcfg);
    if (err < 0)
    {
        ERROR_PRINT(Error parsing base options)
        printCliOptions(&baseopts);
        goto main_out;
    }
    if (runcfg->help && (blength(runcfg->testname) + blength(runcfg->pttfile)) == 0)
    {
        printCliOptions(&baseopts);
        goto main_out;
    }
    if (runcfg->verbosity > 0)
    {
        DEBUG_PRINT(DEBUGLEV_DEVELOP, Setting verbosity to %d, runcfg->verbosity);
        global_verbosity = runcfg->verbosity;
    }

    if (blength(runcfg->testname) > 0)
    {
        got_testcase = 1;
    }
    else
    {
        printCliOptions(&baseopts);
        goto main_out;
    }

    err = read_yaml_ptt(bdata(runcfg->pttfile), &runcfg->tcfg);
    if (err < 0)
    {
        ERROR_PRINT(Error reading %s, bdata(runcfg->pttfile));
        goto main_out;
    }

    struct bstrList* flist = bstrListCreate();
    int res = get_feature_flags(1, &flist);
    if (res < 0)
    {
        ERROR_PRINT(Error reading Flags);
        bstrListDestroy(flist);
        goto main_out;
    }

    res = parse_cpu_folders();
    if (res < 0)
    {
        ERROR_PRINT(Error parsing CPU folders);
        goto main_out;
    }

    // printf("flags qty: %d\n", runcfg->tcfg->flags->qty);
    int found = 0;
    if (runcfg->tcfg->flags->qty > 0 && flist->qty > 0)
    {
        for (int i = 0; i < runcfg->tcfg->flags->qty; i++)
        {
            btrimws(runcfg->tcfg->flags->entry[i]);
            for (int j = 0; j < flist->qty; j++)
            {
                btrimws(flist->entry[j]);
                if (bstrnicmp(runcfg->tcfg->flags->entry[i], flist->entry[j], blength(runcfg->tcfg->flags->entry[i])) == BSTR_OK && blength(runcfg->tcfg->flags->entry[i]) > 0)
                {
                    DEBUG_PRINT(DEBUGLEV_DEVELOP, Flag %s found on the system, bdata(runcfg->tcfg->flags->entry[i]));
                    found++;
                }

            }

        }

    }

    if (found != runcfg->tcfg->flags->qty)
    {
        ERROR_PRINT(Flags not found on the system);
        bstrListPrint(runcfg->tcfg->flags);
        bstrListDestroy(flist);
        goto main_out;
    }

    bstrListDestroy(flist);

    bstring title = bformat("Commandline options for kernel '%s'", bdata(runcfg->testname));
    cliOptionsTitle(&testopts, title);
    bdestroy(title);

    generateTestCliOptions(&testopts, runcfg);

    if (runcfg->help && got_testcase)
    {
        printCliOptions(&baseopts);
        printCliOptions(&testopts);
        goto main_out;
    }
    
    /*
     * Assign & check if all required benchmark parameters are available
     */
    parseCliOptions(args, &testopts);
    err = assignTestCliOptions(&testopts, runcfg);
    if (err < 0)
    {
        ERROR_PRINT(Error assigning runtime CLI test options);
        goto main_out;
    }
    else if (err > 0)
    {
        ERROR_PRINT(Error not all required test parameters set);
        goto main_out;
    }

    if (runcfg->tcfg->requirewg)
    {
        err = assignWorkgroupCliOptions(&testopts, runcfg);
        if (err < 0)
        {
            errno = -err;
            ERROR_PRINT(Error assigning workgroup CLI options);
            goto main_out;
        }
        if (runcfg->num_wgroups == 0) {
            errno = EINVAL;
            ERROR_PRINT(No workgroups on the command line);
            goto main_out;
        }
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
    err = init_result(&runcfg->global_results);
    if (err < 0)
    {
        ERROR_PRINT(Error initializing global result storage);
        goto main_out;
    }
    err = fill_results(runcfg);
    if (err < 0)
    {
        ERROR_PRINT(Error filling result storages);
        goto main_out;
    }

    /*
     * Init arrays
     */
    for (int i = 0; i < runcfg->num_wgroups; i++)
    {
    /*
     * Allocate arrays
     */
        // move allocate stream per wg
        RuntimeWorkgroupConfig* wg = &runcfg->wgroups[i];
        err = manage_streams(wg, runcfg);
        if (err < 0)
        {
            ERROR_PRINT(Error allocating streams);
            goto main_out;
        }
        // do if only global_initialization
        if (runcfg->tcfg->initialization)
        {
            DEBUG_PRINT(DEBUGLEV_DEVELOP, Global Initializing arrays);
            for (int w = 0; w < wg->num_streams; w++)
            {
                DEBUG_PRINT(DEBUGLEV_DEVELOP, Working on stream %s, bdata(wg->streams[w].name));
                err = initialize_arrays(wg->streams[w].ptr);
                if (err < 0)
                {
                    ERROR_PRINT(Error Intializing threads);
                    goto main_out;
                }
            }
        }
    }

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
     * Start threads
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
    destroyCliOptions(&baseopts);
    destroyCliOptions(&testopts);
    if (kernelfolder)
    {
        bdestroy(kernelfolder);
    }
    if (tmpfolder)
    {
        bdestroy(tmpfolder);
    }
    if (args)
    {
        bstrListDestroy(args);
    }
    DEBUG_PRINT(DEBUGLEV_DEVELOP, MAIN_OUT DONE);
    return 0;
}
