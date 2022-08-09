#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/utsname.h>

#include <error.h>
#include <bstrlib.h>
#include <bstrlib_helper.h>

//#include <getopt.h>

#include <test_types.h>
#include <getopt_extra.h>
#include <read_yaml_ptt.h>

//#include <workgroups.h>

#ifndef global_verbosity
int global_verbosity = DEBUGLEV_DEVELOP;
#endif

static struct option_extra baseopts[5] = {
    {"help", 'h', RETURN_TYPE_MASK(RETURN_TYPE_BOOL), 0, "Help and usage"},
    {"test", 't', RETURN_TYPE_MASK(RETURN_TYPE_BSTRING), required_argument, "Test case to run"},
    {"file", 'f', RETURN_TYPE_MASK(RETURN_TYPE_BSTRING), required_argument, "Test file run run", ARG_FLAG_MASK(ARG_FLAG_FILE)},
    {"kfolder", 'd', RETURN_TYPE_MASK(RETURN_TYPE_BSTRING), required_argument, "Directory with benchmarks", ARG_FLAG_MASK(ARG_FLAG_DIR)},
    {0, 0, 0, 0}
};

static struct option_extra basetestopts[4] = {
    {"iters", 'i', RETURN_TYPE_MASK(RETURN_TYPE_UINT64), required_argument, "Number of test case executions"},
    {"workgroup", 'w', RETURN_TYPE_MASK(RETURN_TYPE_BSTRING)|RETURN_TYPE_MASK(RETURN_TYPE_LIST), required_argument, "Workgroup definition (Threads + Streams)"},
    {"runtime", 'r', RETURN_TYPE_MASK(RETURN_TYPE_UINT64), required_argument, "Minimal runtime of a test case (use instead of -i/--iters)", ARG_FLAG_MASK(ARG_FLAG_TIME)},
    {0, 0, 0, 0}
};


static void usage_print_header()
{
    printf("###########################################\n");
    printf("# likwid-bench - Micro-benchmarking suite #\n");
    printf("###########################################\n\n");
}

static void usage_print_baseopts(int message)
{
    printf(" -h/--help          : Print usage\n");
    printf(" -t/--test <str>    : Name of testcase\n");
    printf(" -f/--file <str>    : Filename for testcase\n");
    printf(" -d/--kfolder <str> : Benchmark folder\n");
    if (message)
    {
        printf("\n");
        printf("With the testcase name or the filename, the options might expand if the benchmark requires more input\n");
        printf("\n");
    }
}

static void usage_print_basetestopts()
{
    printf(" -w/--workgroup <str>  : A workgroup definition like S0:2 (multiple options allowed)\n");
    printf(" -i/--iterations <str> : Number of iterations for execution\n");
    printf(" -r/--runtime <time>   : Runtime of benchmark (default 1s, automatically determines iterations)\n");
    printf("\n");
    printf("likwid-bench automatically detects the number of iterations (if not given) for the given or default\n");
    printf("runtime.\n");
    printf("\n");
}


/*static void read_array_option(bstring opt, bstring name, bstring datatype, struct bstrList* codelist , struct bstrList* calllist)*/
/*{*/
/*    FILE* fp = NULL;*/
/*    bstring func = bstrcpy(opt);*/
/*    bstring args = bfromcstr("");*/
/*    bstring bcolon = bfromcstr(":");*/
/*    bstring bslash = bfromcstr("/");*/
/*    bstring bname = bfromcstr("<NAME>");*/
/*    bstring bdatatype = bfromcstr("<DTYPE>");*/

/*    int open = bstrchrp(func, '(', 0);*/
/*    if (open != BSTR_ERR)*/
/*    {*/
/*        int close = bstrchrp(func, ')', open);*/
/*        bdestroy(args);*/
/*        args = bmidstr(func, open, close);*/
/*        btrunc(func, open);*/
/*        btrimbrackets(args);*/
/*    }*/
/*    else*/
/*    {*/
/*        open = 0;*/
/*    }*/

/*    bstring fname = bstrcpy(func);*/
/*    int start = binstrrcaseless(func, blength(func), bcolon);*/
/*    if (start != BSTR_ERR)*/
/*    {*/
/*        bdelete(func, 0, start+1);*/
/*    }*/

/*    bfindreplacecaseless(fname, bcolon, bslash, 0);*/
/*    bcatcstr(fname, ".c");*/
/*    bstring code = read_file(bdata(fname));*/


/*    bfindreplacecaseless(code, bname, name, 0);*/
/*    bfindreplacecaseless(code, bdatatype, datatype, 0);*/


/*    if (blength(code) > 0)*/
/*    {*/
/*        bstrListAdd(codelist, code);*/
/*    }*/

/*    if (calllist)*/
/*    {*/
/*        bstring call = bformat("ret = %s_%s(%s, %s_LEN", bdata(func),*/
/*                                                         bdata(name),*/
/*                                                         bdata(name),*/
/*                                                         bdata(name));*/
/*        if (blength(args) > 0)*/
/*        {*/
/*            bstring t = bformat(", %s", bdata(args));*/
/*            bconcat(call, t);*/
/*            bdestroy(t);*/
/*        }*/
/*        bcatcstr(call, ");");*/
/*        bstrListAdd(calllist, call);*/
/*        bdestroy(call);*/
/*    }*/



/*    bdestroy(func);*/
/*    bdestroy(args);*/
/*    bdestroy(code);*/
/*    bdestroy(fname);*/
/*    bdestroy(bcolon);*/
/*    bdestroy(bslash);*/
/*    bdestroy(bname);*/
/*    bdestroy(bdatatype);*/
/*}*/


void print_usage()
{
    printf("likwid-bench: Micro-benchmarking suite\n\n");
    printf("likwid-bench -t <test> ... (test-specific options)\n");
    printf("Test-specific options are available when calling\n");
    printf("likwid-bench -t <test> -h\n");
}



void print_test_usage(TestConfig_t cfg)
{
    struct tagbstring bdelim = bsStatic(", ");
    //printf("Parameters for testcase %s : \n", bdata(cfg->name));
    //printf("Description: %s\n\n", bdata(cfg->description));
    if (cfg->num_params > 0)
    {
        for (int i = 0; i < cfg->num_params; i++)
        {
            if (blength(cfg->params[i].name) == 1)
            {
                printf(" -%s <value> : %s (Options: ", bdata(cfg->params[i].name), bdata(cfg->params[i].description));
            }
            else
            {
                printf(" --%s <value> : %s (Options: ", bdata(cfg->params[i].name), bdata(cfg->params[i].description));
            }
            bstring opts = bjoin(cfg->params[i].options, &bdelim);
            printf("%s)\n", bdata(opts));
            bdestroy(opts);
        }
    }
    else
    {
        printf("\nTestcase specifies no further options\n");
    }
    printf("\n");
}

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



int main(int argc, char** argv)
{
    int c = 0, err = 0, print_help = 0;
    int option_index = -1;
    //BenchmarkConfig *cfg = NULL;
    RuntimeConfig* runcfg = NULL;
    Map_t useropts = NULL;
    bstring yamlfile = bfromcstr("");
    bstring testcase = bfromcstr("no-benchmark-given-so-it-does-not-exist");
    //bstring bbaseopts = bfromcstr(baseopts_short);
    //bstring bbasetestopts = bfromcstr(basetestopts_short);
    struct tagbstring brequired = bsStatic("required");

    struct tagbstring boptional = bsStatic("optional");
    bstring bccflags = bfromcstr("-fPIC -shared");
    bstring arch = get_architecture();
#ifdef LIKWIDBENCH_KERNEL_FOLDER
    bstring kernelfolder = bformat("%s/%s/", TOSTRING(LIKWIDBENCH_KERNEL_FOLDER), bdata(arch));
#else
    bstring kernelfolder = bformat("/tmp/likwid-bench-%d/%s/", getpid(), bdata(arch));
#endif
    int (*ownaccess)(const char*, int) = access;

    if (argc ==  1)
    {
        usage_print_header();
        usage_print_baseopts(1);
        goto main_out;
    }

    /*
     * Prepare short and long options for getopt
     */
    Map_t cliopts = NULL;
    struct option_extra* opts = NULL;
    
    add_option_list((struct option_extra*)baseopts, &opts);
    int num_args = getopt_long_extra(argc, argv, NULL, opts, &cliopts);

    struct option_extra_return* ret_arg = NULL;
    int got_testcase = 0;
    for (int i = 0; i < get_smap_size(cliopts); i++)
    {
        if (get_smap_by_idx(cliopts, i, (void**)&ret_arg) == 0)
        {
            printf("%d %x\n", i, ret_arg->return_flag);
            if (ret_arg->return_flag & RETURN_TYPE_MASK(RETURN_TYPE_BSTRING))
                printf("%s\n", bdata(ret_arg->value.bstrvalue));
            else if (ret_arg->return_flag & RETURN_TYPE_MASK(RETURN_TYPE_BOOL))
                printf("%s\n", (ret_arg->value.boolvalue ? "true" : "false"));
        }
    }
    if (get_smap_by_key(cliopts, "kfolder", (void**)&ret_arg) == 0)
    {
        bdestroy(kernelfolder);
        kernelfolder = bstrcpy(ret_arg->value.bstrvalue);
    }
    if (get_smap_by_key(cliopts, "test", (void**)&ret_arg) == 0)
    {
        bdestroy(testcase);
        bdata(ret_arg->value.bstrvalue);
        testcase = bstrcpy(ret_arg->value.bstrvalue);
        bdestroy(yamlfile);
        yamlfile = bformat("%s/%s.yaml", bdata(kernelfolder), bdata(testcase));
        got_testcase = 1;
    }
    if (get_smap_by_key(cliopts, "file", (void**)&ret_arg) == 0)
    {
        struct tagbstring bdot = bsStatic(".");
        bdestroy(yamlfile);
        yamlfile = bstrcpy(ret_arg->value.bstrvalue);
        bdestroy(testcase);
        testcase = bformat("%s", basename(bdata(yamlfile)));
        int dot = binstrr(testcase, blength(testcase)-1, &bdot);
        if (dot != BSTR_ERR && dot >= 0)
        {
            btrunc(testcase, dot);
        }
        got_testcase = 1;
    }
    printf("Got testcase %s\n", bdata(testcase));
    if (get_smap_by_key(cliopts, "help", (void**)&ret_arg) == 0)
    {
        if (!got_testcase)
        {
            usage_print_header();
            usage_print_baseopts(1);
            usage_print_basetestopts();
            destroy_smap(cliopts);
            free(opts);
            goto main_out;
        }
        print_help = 1;
    }
    destroy_smap(cliopts);


    TestConfig_t tcfg = NULL;
    printf("read_yaml_ptt %s\n", bdata(yamlfile));
    err = read_yaml_ptt(bdata(yamlfile), &tcfg);
    if (err < 0)
    {
        fprintf(stderr, "Error reading %s\n", bdata(yamlfile));
        if (tcfg)
        {
            free(tcfg);
        }
        free(opts);
        goto main_out;
    }
    
    
    if (print_help && got_testcase)
    {
        usage_print_header();
        usage_print_baseopts(0);
        usage_print_basetestopts();
        print_test_usage(tcfg);
        close_yaml_ptt(tcfg);
        free(opts);
        goto main_out;
    }
    printf("tcfg %p\n", tcfg);
    runcfg = malloc(sizeof(RuntimeConfig));
    if (!runcfg)
    {
        fprintf(stderr, "Error allocating space for runtime configuration\n");
        close_yaml_ptt(tcfg);
        free(opts);
        goto main_out;
    }
    memset(runcfg, 0, sizeof(RuntimeConfig));
    uint64_t iterations = -1;
    double runtime = -1;
    add_option_list((struct option_extra*)basetestopts, &opts);
    struct tagbstring bbytes = bsStatic("bytes");
    struct tagbstring btime = bsStatic("time");
    for (int i = 0; i < tcfg->num_params; i++)
    {
        TestConfigParameter *p = &tcfg->params[i];
        int rflags = RETURN_TYPE_MASK(RETURN_TYPE_UINT64);
        int aflags = 0x0;
        for (int j = 0; j < p->options->qty; j++)
        {
            if (bstrnicmp(p->options->entry[j], &bbytes, blength(&bbytes)) == BSTR_OK)
            {
                aflags = ARG_FLAG_MASK(ARG_FLAG_BYTES);
            }
            else if (bstrnicmp(p->options->entry[j], &btime, blength(&btime)) == BSTR_OK)
            {
                aflags = ARG_FLAG_MASK(ARG_FLAG_TIME);
            }
        }
        printf("Add %s with arg flags 0x%x and return flags 0x%x\n", bdata(p->name), aflags, rflags);
        add_option_params(bdata(p->name), bchar(p->name, 0), rflags, required_argument, &opts, aflags);
    }
    print_options(opts);
    num_args = getopt_long_extra(argc, argv, NULL, opts, &cliopts);
    if (get_smap_by_key(cliopts, "iters", (void**)&ret_arg) == 0)
    {
        runcfg->iterations = ret_arg->value.uint64value;
    }
    if (get_smap_by_key(cliopts, "runtime", (void**)&ret_arg) == 0)
    {
        runcfg->runtime = ret_arg->value.doublevalue;
    }
    // if (get_smap_by_key(cliopts, "workgroup", (void**)&ret_arg) == 0)
    // {
    //     struct bstrList* wlist = ret_arg->value.bstrlist;
    //     runcfg->wgroups = malloc(wlist->qty * sizeof(RuntimeWorkgroupConfig));
    //     if (!runcfg->wgroups)
    //     {
    //         destroy_smap(cliopts);
    //         free(opts);
    //         close_yaml_ptt(tcfg);
    //         goto main_out;
    //     }
    //     runcfg->num_wgroups = 0;
    //     for (int i = 0; i < wlist->qty; i++)
    //     {
    //         err = parse_workgroup(wlist->entry[i], &runcfg->wgroups[runcfg->num_wgroups]);
    //         if (err != 0)
    //         {
    //             free(runcfg->wgroups);
    //             destroy_smap(cliopts);
    //             free(opts);
    //             close_yaml_ptt(tcfg);
    //             goto main_out;
    //         }
    //         runcfg->num_wgroups++;
    //     }
    // }
    int required_missing = 0;
    for (int i = 0; i < tcfg->num_params; i++)
    {
        int is_required = 0;
        TestConfigParameter *p = &tcfg->params[i];
        for (int j = 0; j < p->options->qty; j++)
        {
            printf("%s\n", bdata(p->options->entry[j]));
            if (bstrnicmp(p->options->entry[j], &brequired, blength(&brequired)) == BSTR_OK)
            {
                is_required = 1;
                break;
            }
        }
        if (get_smap_by_key(cliopts, bdata(p->name), (void**)&ret_arg) == 0)
        {
            if (is_required)
            {
                printf("Got required test param '%s'\n", bdata(p->name));
            }
            else
            {
                printf("Got test param '%s'\n", bdata(p->name));
            }
        }
        else
        {
            fprintf(stderr, "Missing required parameter '%s'\n", bdata(p->name));
            required_missing++;
        }
    }
    destroy_smap(cliopts);
    free(opts);
    close_yaml_ptt(tcfg);
    
    goto main_out;



    /*
     * Check if all required benchmark parameters are available
     */
/*    if (num_required_params > 0)*/
/*    {*/
/*        printf("Required parameter missing %d\n", num_required_params);*/
/*        for (int i = 0; i < cfg->num_params; i++)*/
/*        {*/
/*            bstring name;*/
/*            bstring desc;*/
/*            bstring def;*/
/*            if (!get_smap_by_key(cfg->parameters[i], "name", (void**)&name))*/
/*            {*/
/*                void* tmp = NULL;*/
/*                if (get_smap_by_key(runcfg->useropts, bdata(name), &tmp))*/
/*                {*/
/*                    printf("Name: %s\n", bdata(name));*/
/*                    if (!get_smap_by_key(cfg->parameters[i], "default", (void**)&def))*/
/*                    {*/
/*                        printf("Parameter has default value: %s\n", bdata(def));*/
/*                        add_smap(runcfg->useropts, bdata(name), bstrcpy(def));*/
/*                    }*/
/*                    else*/
/*                    {*/
/*                        printf("Option '%s' required", bdata(name));*/
/*                        if (!get_smap_by_key(cfg->parameters[i], "description", (void**)&desc))*/
/*                        {*/
/*                            printf(": %s", bdata(desc));*/
/*                        }*/
/*                        printf("\n");*/
/*                    }*/
/*                }*/
/*                else*/
/*                {*/
/*                    printf("Oops!?\n");*/
/*                }*/
/*            }*/
/*        }*/
/*    }*/

    //int level = 0;
    //foreach_in_smap(runcfg->useropts, print_map_elem, &level);
/*    update_benchmark_config(cfg, runcfg->useropts);*/

    /*
     * Analyse workgroups
     */
/*    int num_workgroups = create_workgroups(runcfg, workgroups);*/
/*    bstrListDestroy(workgroups);*/

    /*
     * Generate assembly
     */

    /*
     * Allocate arrays
     */

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
    bdestroy(bccflags);
    bdestroy(testcase);
    bdestroy(yamlfile);
/*    bdestroy(bbaseopts);*/
/*    bdestroy(bbasetestopts);*/
    // bdestroy(boptional);
    // bdestroy(brequired);
    bdestroy(arch);
    bdestroy(kernelfolder);
/*    if (runcfg)*/
/*        destroy_runtime_config(runcfg);*/
/*    if (cfg)*/
/*        destroy_benchmark_config(cfg);*/
    return 0;
}
