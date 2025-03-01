#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/utsname.h>

#include "error.h"
#include "bstrlib.h"
#include "bstrlib_helper.h"
#include "calculator.h"
#include "test_types.h"
#include "read_yaml_ptt.h"
#include "cli_parser.h"
#include "workgroups.h"
#include "ptt2asm.h"
#include "allocator.h"
#include "results.h"
#include "topology.h"
#include "thread_group.h"
#include "dynload.h"
#include "table.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef LIKWID_PERFMON
#include "likwid-marker.h"
#endif

#ifdef __cplusplus
extern "C" }
#endif

#ifndef global_verbosity
int global_verbosity = DEBUGLEV_ONLY_ERROR;
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

static bstring hline()
{
    bstring bdash = bfromcstr("");
    binsertch(bdash, 0, 80, '-');
    binsertch(bdash, 80, 1, '\n');
    return bdash;
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
    runcfg->compiler = bfromcstr("");
    runcfg->iterations = 0;
    runcfg->runtime = -1.0;
    runcfg->csv = 0;
    runcfg->json = 0;
    runcfg->output = bfromcstr("stdout");
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
        DEBUG_PRINT(DEBUGLEV_DEVELOP, Destroy compiler in RuntimeConfig);
        bdestroy(runcfg->compiler);
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
                        if (runcfg->wgroups[i].threads)
                        {
                            if (runcfg->wgroups[i].threads[j].data)
                            {
                                free(runcfg->wgroups[i].threads[j].data);
                                runcfg->wgroups[i].threads[j].data = NULL;
                            }
                            if (runcfg->wgroups[i].threads[j].command)
                            {
                                if (runcfg->wgroups[i].threads[j].command->init_val)
                                {
                                    free(runcfg->wgroups[i].threads[j].command->init_val);
                                    runcfg->wgroups[i].threads[j].command->init_val = NULL;
                                }
                                free(runcfg->wgroups[i].threads[j].command);
                                runcfg->wgroups[i].threads[j].command = NULL;
                            }
                        }
                    }
                    free(runcfg->wgroups[i].threads);
                    destroy_result(runcfg->wgroups[i].group_results);
                    free(runcfg->wgroups[i].group_results);
                    runcfg->wgroups[i].group_results = NULL;
                    free(runcfg->wgroups[i].results);
                    runcfg->wgroups[i].results = NULL;
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
                if (runcfg->wgroups[i].testconfig.objfile)
                {
                    bdestroy(runcfg->wgroups[i].testconfig.objfile);
                }
                if (runcfg->wgroups[i].testconfig.compiler)
                {
                    bdestroy(runcfg->wgroups[i].testconfig.compiler);
                }
                if (runcfg->wgroups[i].testconfig.flags)
                {
                    bdestroy(runcfg->wgroups[i].testconfig.flags);
                }
                if (runcfg->wgroups[i].testconfig.functionname)
                {
                    bdestroy(runcfg->wgroups[i].testconfig.functionname);
                }
                if (runcfg->wgroups[i].testconfig.function)
                {
                    close_function(&runcfg->wgroups[i]);
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

        if (runcfg->global_results)
        {
            if (runcfg->global_results->variables)
            {
                destroy_result(runcfg->global_results);
                free(runcfg->global_results);
                runcfg->global_results = NULL;
            }
        }

        bdestroy(runcfg->output);
        free(runcfg);
    }
}

int main(int argc, char** argv)
{
#ifdef LIKWID_PERFMON
    if (getenv("LIKWID_FILEPATH") != NULL)
    {
        printf("Using LIKWID_MARKER API\n");
    }
    LIKWID_MARKER_INIT;
#endif
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

    int precision = DEFAULTPRECISION + 1;
    calculator_init();
    calculator_setprecision(precision);
    bstring arch = get_architecture();
#ifdef LIKWIDBENCH_KERNEL_FOLDER
    bstring kernelfolder = bformat("%s/%s/", TOSTRING(LIKWIDBENCH_KERNEL_FOLDER), bdata(arch));
#else
    bstring kernelfolder = bformat("./");
#endif
    bstring tmpfolder = bformat("/tmp/likwid-bench-%d/%s/", getpid(), bdata(arch));
    bstring compiler = bfromcstr("gcc");
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
    bconcat(runcfg->compiler, compiler);

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
                if (blength(runcfg->tcfg->flags->entry[i]) > 0 && biseq(runcfg->tcfg->flags->entry[i], flist->entry[j]))
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

    printf("%s", bdata(hline()));
    printf("Application: LIKWID-BENCH\n");
    printf("Test: %s\n", bdata(runcfg->testname));
    for (int w = 0; w < runcfg->num_wgroups; w++)
    {
        printf("Using %d work groups\n", runcfg->num_wgroups);
        RuntimeWorkgroupConfig* wg = &runcfg->wgroups[w];
        printf("Using %d threads\n", wg->num_threads);
    }
    printf("%s", bdata(hline()));

    /*
     * Analyse workgroups
     */
    err = resolve_workgroups(runcfg->detailed, runcfg->num_wgroups, runcfg->wgroups);
    if (err < 0)
    {
        ERROR_PRINT(Error resolving workgroups);
        goto main_out;
    }

    /*
     * Evaluate variables, constants, ... for remaining operations
     * There should be now all values available
     */
    runcfg->global_results = malloc(sizeof(RuntimeWorkgroupResult));
    if (!runcfg->global_results)
    {
        ERROR_PRINT(Unable to allocate memory for global results);
        goto main_out;
    }
    err = init_result(runcfg->global_results);
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
        RuntimeWorkgroupConfig* wg = &runcfg->wgroups[i];
        // move allocate stream per wg
        err = manage_streams(wg, runcfg);
        if (err < 0)
        {
            ERROR_PRINT(Error allocating streams);
            goto main_out;
        }
    }

    /*
     * Generate assembly
     */
    runcfg->codelines = bstrListCreate();
    err = generate_code(runcfg, runcfg->codelines);
    if (err < 0)
    {
        ERROR_PRINT(Error generating code);
        goto main_out;
    }
    for (int i = 0; i < runcfg->codelines->qty; i++)
    {
        DEBUG_PRINT(DEBUGLEV_DETAIL, "CODE: %s\n", bdata(runcfg->codelines->entry[i]));
    }

    /*
     * Start threads
     */
    err = update_threads(runcfg);
    if (err < 0)
    {
        ERROR_PRINT(Error updating thread groups);
        goto main_out;
    }

    /*
     * Update kernel code
     */
    for (int i = 0; i < runcfg->num_wgroups; i++)
    {
        err = dynload_create_runtime_test_config(runcfg, &runcfg->wgroups[i]);
        if (err < 0)
        {
            ERROR_PRINT(Error generating function object);
            goto main_out;
        }
        err = open_function(&runcfg->wgroups[i]);
        if (err < 0)
        {
            ERROR_PRINT(Error opening function objects);
            goto main_out;
        }
    }

    /*
     * Prepare thread runtime info
     */
    err = create_threads(runcfg->num_wgroups, runcfg->wgroups);
    if (err < 0)
    {   
        ERROR_PRINT(Error creating thread);
        destroy_threads(runcfg->num_wgroups, runcfg->wgroups);
        goto main_out;
    }

    /* Send LIKWID CMD's */
    for (int w = 0; w < runcfg->num_wgroups; w++)
    {
        RuntimeWorkgroupConfig* wg = &runcfg->wgroups[w];
        for (int i = 0; i < wg->num_threads; i++)
        {
            err = send_cmd(LIKWID_THREAD_COMMAND_INITIALIZE, &wg->threads[i]);
            if (err < 0)
            {
                ERROR_PRINT(Error communicating with threads);
                destroy_threads(runcfg->num_wgroups, runcfg->wgroups);
                goto main_out;
            }
        }
    }

    /*
     * Run benchmark
     */
    for (int w = 0; w < runcfg->num_wgroups; w++)
    {
        RuntimeWorkgroupConfig* wg = &runcfg->wgroups[w];
        for (int i = 0; i < wg->num_threads; i++)
        {
            RuntimeThreadConfig* thread =  &wg->threads[i];
            DEBUG_PRINT(DEBUGLEV_DEVELOP, Setting thread %d run command function to %p, thread->local_id, wg->testconfig.function);
            thread->command->cmdfunc.run = wg->testconfig.function;
            err = send_cmd(LIKWID_THREAD_COMMAND_RUN, thread);
            if (err < 0)
            {
                ERROR_PRINT(Error communicating with threads);
                destroy_threads(runcfg->num_wgroups, runcfg->wgroups);
                goto main_out;
            }
        }
    }

    /*
     * Exit threads
     */
    for (int w = 0; w < runcfg->num_wgroups; w++)
    {
        RuntimeWorkgroupConfig* wg = &runcfg->wgroups[w];
        for (int i = 0; i < wg->num_threads; i++)
        {
            err = send_cmd(LIKWID_THREAD_COMMAND_EXIT, &wg->threads[i]);
            if (err < 0)
            {
                ERROR_PRINT(Error communicating with threads);
                destroy_threads(runcfg->num_wgroups, runcfg->wgroups);
                goto main_out;
            }
        }
    }

    err = join_threads(runcfg->num_wgroups, runcfg->wgroups);
    if (err < 0)
    {
        ERROR_PRINT(Error joining threads);
        goto main_out;
    }

    err = update_results(runcfg, runcfg->num_wgroups, runcfg->wgroups);
    if (err != 0)
    {
        ERROR_PRINT(Error updating results);
    }

    /*
     * Free arrays
     */
    release_streams(runcfg->num_wgroups, runcfg->wgroups);

    /*
     * Destroy threads
     */
    err = destroy_threads(runcfg->num_wgroups, runcfg->wgroups);
    if (err != 0)
    {
        ERROR_PRINT(Error destroying thread groups);
        goto main_out;
    }

    /*
     * * Calculate metrics
     */

    /*
     * * Print everything
     */
    if (DEBUGLEV_DEVELOP == global_verbosity)
    {
        printf("Workgroup Results\n");
        for (int i = 0; i < runcfg->num_wgroups; i++)
        {
            print_workgroup(&runcfg->wgroups[i]);
        }

        printf("Global Results\n");
        print_result(runcfg->global_results);
    }

    Table* thread = NULL;
    Table* wgroup = NULL;
    Table* global = NULL;
    int max_cols = 0;
    update_table(runcfg, &thread, &wgroup, &global, &max_cols, 1);
    FILE* output = NULL;
    int fileout = 0;
    if (blength(runcfg->output) > 0)
    {
        struct tagbstring bstdout = bsStatic("stdout");
        struct tagbstring bstderr = bsStatic("stderr");
        if (bstrnicmp(runcfg->output, &bstdout, blength(&bstdout)) == BSTR_OK)
        {
            output = stdout;
        }
        else if (bstrnicmp(runcfg->output, &bstdout, blength(&bstdout)) == BSTR_OK)
        {
            output = stderr;
        }
        else
        {
            output = fopen(bdata(runcfg->output), "a");
            if (!output)
            {
                fprintf(stderr, "Cannot open file %s to write results. Using stdout.\n", bdata(runcfg->output));
                output = stdout;
            }
            else
            {
                fileout = 1;
            }
        }
    }

    if (runcfg->csv == 0 && runcfg->json == 0)
    {
        fprintf(output, "Thread Results\n");
        table_print(output, thread, 1);
        fprintf(output, "\nWorkgroup Results\n");
        table_print(output, wgroup, 1);
        fprintf(output, "\nGlobal Results\n");
        table_print(output, global, 1);
    }
    else if (runcfg->csv > 0)
    {
        table_to_csv(output, thread, bdata(runcfg->output), max_cols, 1);
        table_to_csv(output, wgroup, bdata(runcfg->output), max_cols, 1);
        table_to_csv(output, global, bdata(runcfg->output), max_cols, 1);
    }
    else if (runcfg->json > 0)
    {
        table_to_json(output, thread, bdata(runcfg->output), "thread_results");
        table_to_json(output, wgroup, bdata(runcfg->output), "workgroup_results");
        table_to_json(output, global, bdata(runcfg->output), "global_results");
    }
    table_destroy(thread);
    table_destroy(wgroup);
    table_destroy(global);

    if (fileout && output)
    {
        fclose(output);
    }

#ifdef LIKWID_PERFMON
    if (getenv("LIKWID_FILEPATH") != NULL)
    {
        printf("Writing LIKWID_MARKER API results to file %s\n", getenv("LIKWID_FILEPATH"));
    }
    LIKWID_MARKER_CLOSE;
#endif

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
    if (compiler)
    {
        bdestroy(compiler);
    }
    if (args)
    {
        bstrListDestroy(args);
    }
    DEBUG_PRINT(DEBUGLEV_DEVELOP, MAIN_OUT DONE);
    return 0;
}
