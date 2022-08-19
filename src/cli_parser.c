#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>


#include "error.h"
#include "test_types.h"
#include "getopt_extra.h"
#include "cli_parser.h"



void usage_print_baseopts(int message)
{
/*    printf(" -h/--help          : Print usage\n");*/
/*    printf(" -t/--test <str>    : Name of testcase\n");*/
/*    printf(" -f/--file <str>    : Filename for testcase\n");*/
/*    printf(" -d/--kfolder <str> : Benchmark folder\n");*/
    print_options(baseopts);
    if (message)
    {
        printf("\n");
        printf("With the testcase name or the filename, the options might expand if the benchmark requires more input\n");
        printf("\n");
    }
}

void usage_print_basetestopts()
{
/*    printf(" -w/--workgroup <str>  : A workgroup definition like S0:2 (multiple options allowed)\n");*/
/*    printf(" -i/--iterations <str> : Number of iterations for execution\n");*/
/*    printf(" -r/--runtime <time>   : Runtime of benchmark (default 1s, automatically determines iterations)\n");*/
    print_options(basetestopts);
    printf("\n");
    printf("likwid-bench automatically detects the number of iterations (if not given) for the given or default\n");
    printf("runtime.\n");
    printf("\n");
}


void print_usage()
{
    printf("likwid-bench: Micro-benchmarking suite\n\n");
    printf("likwid-bench -t <test> ... (test-specific options)\n");
    printf("Test-specific options are available when calling\n");
    printf("likwid-bench -t <test> -h\n");
}

void usage_print_header()
{
    printf("###########################################\n");
    printf("# likwid-bench - Micro-benchmarking suite #\n");
    printf("###########################################\n\n");
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

int parse_baseopts(int argc, char* argv[], RuntimeConfig* config)
{
    Map_t cliopts;
    struct option_extra* opts = NULL;
    struct option_extra_return* ret_arg = NULL;
    add_option_list((struct option_extra*)baseopts, &opts);
    int option_index = -1;
    int num_args = getopt_long_extra(argc, argv, NULL, opts, &cliopts);
    if (num_args < 0)
    {
        return num_args;
    }
    if (get_smap_by_key(cliopts, "help", (void**)&ret_arg) == 0)
    {
        config->help = 1;
    }
    if (get_smap_by_key(cliopts, "test", (void**)&ret_arg) == 0)
    {
        if (ret_arg->return_flag & RETURN_TYPE_MASK(RETURN_TYPE_BSTRING))
        {
            btrunc(config->testname, 0);
            bconcat(config->testname, ret_arg->value.bstrvalue);
        }
    }
    if (get_smap_by_key(cliopts, "file", (void**)&ret_arg) == 0)
    {
        if (ret_arg->return_flag & RETURN_TYPE_MASK(RETURN_TYPE_BSTRING))
        {
            btrunc(config->pttfile, 0);
            bconcat(config->pttfile, ret_arg->value.bstrvalue);
        }
    }
    if (get_smap_by_key(cliopts, "kfolder", (void**)&ret_arg) == 0)
    {
        if (ret_arg->return_flag & RETURN_TYPE_MASK(RETURN_TYPE_BSTRING))
        {
            btrunc(config->tmpfolder, 0);
            bconcat(config->tmpfolder, ret_arg->value.bstrvalue);
        }
    }
    destroy_smap(cliopts);
    free(opts);
    return 0;
}

int parse_testopts(int argc, char* argv[], TestConfig* tcfg, RuntimeConfig* config)
{
    int err = 0;
    Map_t cliopts;
    struct option_extra* opts = NULL;
    struct option_extra_return* ret_arg = NULL;

    DEBUG_PRINT(DEBUGLEV_DEVELOP, Adding base CLI options);
    add_option_list((struct option_extra*)baseopts, &opts);
    DEBUG_PRINT(DEBUGLEV_DEVELOP, Adding test-related CLI options);
    add_option_list((struct option_extra*)basetestopts, &opts);

    struct tagbstring bbytes = bsStatic("bytes");
    struct tagbstring btime = bsStatic("time");
    struct tagbstring bEOF = bsStatic("EOF");
    for (int i = 0; i < tcfg->num_params; i++)
    {
        TestConfigParameter *p = &tcfg->params[i];
        int rflags = 0x0;
        int aflags = 0x0;
        for (int j = 0; j < p->options->qty; j++)
        {
            int k = 0;
            while (bstrcmp(&param_opts[k].name, &bEOF) != BSTR_OK)
            {
                if (bstrnicmp(p->options->entry[j], &param_opts[k].name, blength(&param_opts[k].name)) == BSTR_OK)
                {
                    DEBUG_PRINT(DEBUGLEV_DEVELOP, Option %s adding argflags 0x%x and retflags 0x%x, bdata(&param_opts[k].name), param_opts[k].arg_flags, param_opts[k].ret_flags);
                    aflags |= param_opts[k].arg_flags;
                    rflags |= param_opts[k].ret_flags;
                    break;
                }
                k++;
            }
        }
        if (!rflags)
        {
            rflags = RETURN_TYPE_MASK(RETURN_TYPE_BSTRING);
        }
        DEBUG_PRINT(DEBUGLEV_DEVELOP, Adding CLI option %s with arg flags 0x%x and return flags 0x%x, bdata(p->name), aflags, rflags);
        add_option_params(bdata(p->name), bchar(p->name, 0), rflags, required_argument, &opts, aflags);
    }

    int num_args = getopt_long_extra(argc, argv, NULL, opts, &cliopts);
    if (get_smap_by_key(cliopts, "iters", (void**)&ret_arg) == 0)
    {
        if (ret_arg->return_flag & RETURN_TYPE_MASK(RETURN_TYPE_INT))
        {
            config->iterations = ret_arg->value.intvalue;
        }
        else if (ret_arg->return_flag & RETURN_TYPE_MASK(RETURN_TYPE_UINT64))
        {
            config->iterations = ret_arg->value.uint64value;
        }
    }
    if (get_smap_by_key(cliopts, "runtime", (void**)&ret_arg) == 0)
    {
        if (ret_arg->return_flag & RETURN_TYPE_MASK(RETURN_TYPE_INT))
        {
            config->runtime = (double)ret_arg->value.intvalue;
        }
        else if (ret_arg->return_flag & RETURN_TYPE_MASK(RETURN_TYPE_UINT64))
        {
            config->runtime = (double)ret_arg->value.uint64value;
        }
        else if (ret_arg->return_flag & RETURN_TYPE_MASK(RETURN_TYPE_FLOAT))
        {
            config->runtime = (double)ret_arg->value.floatvalue;
        }
        else if (ret_arg->return_flag & RETURN_TYPE_MASK(RETURN_TYPE_DOUBLE))
        {
            config->runtime = ret_arg->value.doublevalue;
        }
    }
    if (get_smap_by_key(cliopts, "workgroup", (void**)&ret_arg) == 0)
    {
        struct bstrList* wlist = ret_arg->value.bstrlist;
        if (wlist && wlist->qty > 0)
        {
            config->wgroups = malloc(wlist->qty * sizeof(RuntimeWorkgroupConfig));
            if (config->wgroups)
            {
                memset(config->wgroups, 0, wlist->qty * sizeof(RuntimeWorkgroupConfig));
                for (int i = 0; i < wlist->qty; i++)
                {
                    RuntimeWorkgroupConfig* wg = &config->wgroups[i];
                    wg->str = bstrcpy(wlist->entry[i]);
                    wg->cpulist = NULL;
                    wg->threads = NULL;
                }
                config->num_wgroups = wlist->qty;
            }
            else
            {
                ERROR_PRINT(Failed to allocate space for workgroups\n);
                err = -ENOMEM;
            }
        }
    }
    int run_params = 0;
    for (int i = 0; i < tcfg->num_params; i++)
    {
        TestConfigParameter *p = &tcfg->params[i];
        if (get_smap_by_key(cliopts, bdata(p->name), (void**)&ret_arg) == 0)
        {
            run_params++;
        }
    }
    config->params = malloc(run_params + sizeof(RuntimeParameterConfig));
    if (!config->params)
    {
        err = -ENOMEM;
    }
    else
    {
        run_params = 0;
        for (int i = 0; i < tcfg->num_params; i++)
        {
            TestConfigParameter *p = &tcfg->params[i];
            if (get_smap_by_key(cliopts, bdata(p->name), (void**)&ret_arg) == 0)
            {
                RuntimeParameterConfig* run = &config->params[run_params];
                run->name = bstrcpy(p->name);
                run->type = RETURN_TYPE_INVALID;
                run->value.uint64value = 0;
                for (int j = 0; j < p->options->qty; j++)
                {
                    int k = 0;
                    while (bstrcmp(&param_opts[k].name, &bEOF) == BSTR_ERR)
                    {
                        if (bstrnicmp(p->options->entry[j], &param_opts[k].name, blength(&param_opts[k].name)) == BSTR_OK)
                        {
                            for (int f = RETURN_TYPE_MIN; f < RETURN_TYPE_MAX - 1; f++)
                            {
                                if (param_opts[k].ret_flags & RETURN_TYPE_MASK(f))
                                {
                                    run->type = f;
                                    break;
                                }
                            }
                        }
                        if (run->type != RETURN_TYPE_INVALID) break;
                        k++;
                    }
                }
                switch (run->type)
                {
                    case RETURN_TYPE_BOOL:
                        run->value.boolvalue = ret_arg->value.boolvalue;
                        break;
                    case RETURN_TYPE_BSTRING:
                        run->value.bstrvalue = bstrcpy(ret_arg->value.bstrvalue);
                        break;
                    case RETURN_TYPE_INT:
                        run->value.intvalue = ret_arg->value.intvalue;
                        break;
                    case RETURN_TYPE_UINT64:
                        run->value.uint64value = ret_arg->value.uint64value;
                        break;
                    case RETURN_TYPE_FLOAT:
                        run->value.floatvalue = ret_arg->value.floatvalue;
                        break;
                    case RETURN_TYPE_DOUBLE:
                        run->value.doublevalue = ret_arg->value.doublevalue;
                        break;
                    case RETURN_TYPE_STRING:
                        run->value.strvalue = malloc((strlen(ret_arg->value.strvalue)+1) * sizeof(char));
                        if (run->value.strvalue)
                        {
                            run->value.strvalue[0] = '\0';
                            int ret = snprintf(run->value.strvalue, strlen(ret_arg->value.strvalue), "%s", ret_arg->value.strvalue);
                            if (ret >= 0)
                            {
                                run->value.strvalue[ret] = '\0';
                            }
                        }
                }
                run_params++;
            }
        }
        config->num_params = run_params;
    }

    free(opts);
    destroy_smap(cliopts);
    return err;
}

int check_required_params(TestConfig* tcfg, RuntimeConfig* config)
{
    int required_missing = 0;
    struct tagbstring brequired = bsStatic("required");
    //struct tagbstring boptional = bsStatic("optional");
    for (int i = 0; i < tcfg->num_params; i++)
    {
        int is_required = 0;
        int is_set = 0;
        TestConfigParameter *p = &tcfg->params[i];
        for (int j = 0; j < p->options->qty; j++)
        {
            if (bstrnicmp(p->options->entry[j], &brequired, blength(&brequired)) == BSTR_OK)
            {
                is_required = 1;
                break;
            }
        }
        for (int j = 0; j < config->num_params; j++)
        {
            if (bstrncmp(p->name, config->params[i].name, blength(p->name)) == BSTR_OK)
            {
                is_set = 1;
                break;
            }
        }
        if (is_set)
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
        else if (is_required)
        {
            fprintf(stderr, "Missing required parameter '%s'\n", bdata(p->name));
            required_missing++;
        }
    }
    return required_missing;
}
