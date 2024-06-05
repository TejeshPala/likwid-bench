#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>

#include "cli_parser.h"
#include "bstrlib.h"
#include "bstrlib_helper.h"
#include "test_types.h"
#include "error.h"

static int copy_cli(int argc, char** argv, char*** copy)
{
    char** c = malloc(argc * sizeof(char*));
    for (int i = 0; i < argc; i++)
    {
        int len = strlen(argv[i]);
        c[i] = malloc((len+2) * sizeof(char));
        int ret = snprintf(c[i], len+1, "%s", argv[i]);
        c[i][ret] = '\0';
    }
    *copy = c;
    return argc;
}

static void free_cli(int argc, char** argv)
{
    for (int i = 0; i < argc; i++)
    {
        free(argv[i]);
    }
    free(argv);
}

/*static struct option baseopts[7] = {*/
/*    {"help", no_argument, 0, 'h'},*/
/*    {"verbose", required_argument, 0, 'V'},*/
/*    {"test", required_argument, 0, 't'},*/
/*    {"file", required_argument, 0, 'f'},*/
/*    {"kfolder", required_argument, 0, 'K'},*/
/*    {"tmpfolder", required_argument, 0, 'D'},*/
/*    {0, 0, 0, 0}*/
/*};*/

/*static struct option basetestopts[3] = {*/
/*    {"iterations", required_argument, 0, 'i'},*/
/*    {"runtime", required_argument, 0, 'r'},*/
/*    {0, 0, 0, 0}*/
/*};*/

/*static struct option wgroupopts[2] = {*/
/*    {"workgroup", required_argument, 0, 'w'},*/
/*    {0, 0, 0, 0}*/
/*}*/




int addCliOption(CliOptions* options, CliOption* new)
{
    int symbol_free = 1;
    int name_free = 1;
    int symbol_candidate_lower = 1;
    int symbol_candidate_upper = 1;
    if ((!options) || (!new))
    {
        return -EINVAL;
    }
    int first_lower = (bchar(new->name, 0) > 'a' && bchar(new->name, 0) < 'z');
    for (int i = 0; i < options->num_options; i++)
    {
        CliOption* x = &options->options[i];
        bstring candidate = bformat("-%c", bchar(new->name, 0));
        btolower(candidate);
        if (bstrcmp(x->name, new->name) == BSTR_OK)
        {
            name_free = 0;
        }
        if (bstrcmp(x->symbol, new->symbol) == BSTR_OK)
        {
            symbol_free = 0;
        }
        if (bstrcmp(x->symbol, candidate) == BSTR_OK)
        {
            symbol_candidate_lower = 0;
        }
        btoupper(candidate);
        if (bstrcmp(x->symbol, candidate) == BSTR_OK)
        {
            symbol_candidate_upper = 0;
        }
        bdestroy(candidate);
    }
    if ((!name_free) && (!symbol_free))
    {
        ERROR_PRINT(Name '%s' and symbol '%s' already taken, bdata(new->name), bdata(new->symbol));
        return -EINVAL;
    }
    CliOption* tmp = realloc(options->options, (options->num_options+1) * sizeof(CliOption));
    if (!tmp)
    {
        return -ENOMEM;
    }
    options->options = tmp;
    CliOption* x = &options->options[options->num_options];
    x->name = bstrcpy(new->name);
    if (symbol_free)
    {
        x->symbol = bstrcpy(new->symbol);
    }
    else
    {
        if (first_lower && symbol_candidate_lower)
        {
            x->symbol = bformat("-%c", bchar(new->name, 0));
        }
        else if ((!first_lower) && symbol_candidate_upper)
        {
            x->symbol = bformat("-%c", bchar(new->name, 0));
        }
        else if (symbol_candidate_lower)
        {
            x->symbol = bformat("-%c", bchar(new->name, 0));
            btolower(x->symbol);
        }
        else if (symbol_candidate_upper)
        {
            x->symbol = bformat("-%c", bchar(new->name, 0));
            btoupper(x->symbol);
        }
        else
        {
            x->symbol = bfromcstr("****");
        }
    }
    x->has_arg = new->has_arg;
    x->description = bfromcstr("");
    if (new->description && blength(new->description) > 0)
    {
        bconcat(x->description, new->description);
    }
    x->value = bfromcstr("");
    if (new->value && blength(new->value) > 0)
    {
        bconcat(x->value, new->value);
    }
    x->values = bstrListCreate();
    options->num_options++;
    return 0;
}

void destroyCliOptions(CliOptions* options)
{
    for (int i = 0; i < options->num_options; i++)
    {
        CliOption* x = &options->options[i];
        bdestroy(x->name);
        bdestroy(x->symbol);
        bdestroy(x->description);
        bdestroy(x->value);
        bstrListDestroy(x->values);
    }
    memset(options->options, 0, options->num_options * sizeof(CliOption));
    free(options->options);
    options->options = NULL;
    options->num_options = 0;
    if (options->title)
    {
        bdestroy(options->title);
    }
    if (options->prolog)
    {
        bdestroy(options->prolog);
    }
    if (options->epilog)
    {
        bdestroy(options->prolog);
    }
}

void printCliOptions(CliOptions* options)
{
    if (!options)
    {
        return;
    }
    if (options->title)
    {
        printf("---------------------------------------\n");
        printf("%s\n", bdata(options->title));
        printf("---------------------------------------\n");
    }
    if (options->prolog)
    {
        printf("%s\n", bdata(options->prolog));
        printf("----\n");
    }
    printf("Options:\n");
    for (int i = 0; i < options->num_options; i++)
    {
        CliOption *x = &options->options[i];
        printf("\t%s/%-20s : %s", bdata(x->symbol), bdata(x->name), bdata(x->description));
        if (blength(x->value) > 0)
        {
            printf(" = '%s'", bdata(x->value));
        }
        if (x->values->qty > 0)
        {
            struct tagbstring bspace = bsStatic("|");
            bstring list = bjoin(x->values, &bspace);
            printf(" = '%s'", bdata(list));
            bdestroy(list);
        }
        printf("\n");
    }
    if (options->epilog)
    {
        printf("----\n");
        printf("%s\n", bdata(options->epilog));
    }
}

int addConstCliOptions(CliOptions* options, ConstCliOptions *constopts)
{
    if ((!options) || (!constopts))
    {
        return -EINVAL;
    }
    int newlen = options->num_options + constopts->num_options;
    CliOption* tmp = realloc(options->options, (newlen) * sizeof(CliOption));
    if (!tmp)
    {
        return -ENOMEM;
    }
    options->options = tmp;

    for (int i = 0; i < constopts->num_options; i++)
    {
        CliOption* x = &options->options[options->num_options + i];
        x->name = bformat("--%s", constopts->options[i].name);
        x->symbol = bformat("-%c", constopts->options[i].symbol);
        x->has_arg = constopts->options[i].has_arg;
        x->description = bfromcstr(constopts->options[i].description);
        x->value = bfromcstr("");
        x->values = bstrListCreate();
    }
    options->num_options += constopts->num_options;
    return 0;
}

static void set_required_argument(CliOption* x, bstring arg)
{
    if (blength(x->value) > 0)
    {
        btrunc(x->value, 0);
    }
    printf("Set argument for %s/%s: %s\n", bdata(x->symbol), bdata(x->name), bdata(arg));
    bconcat(x->value, arg);
}

static void add_multi_argument(bstring combine, bstring arg)
{
    if (blength(combine) > 0)
    {
        bconchar(combine, ' ');
    }
    printf("Add argument for multi_argument %s\n", bdata(arg));
    bconcat(combine, arg);
}

int parseCliOptions(struct bstrList* argv, CliOptions* options)
{
    struct tagbstring btrue = bsStatic("1");
    bstring combine = bfromcstr("");
    CliOption* combine_opt = NULL;
    for (int i = 0; i < argv->qty; i++)
    {
        int processed = 0;
        for (int j = 0; j < options->num_options; j++)
        {
            CliOption* x = &options->options[j];
            if (bstrcmp(argv->entry[i], x->name) == BSTR_OK || bstrcmp(argv->entry[i], x->symbol) == BSTR_OK)
            {
                if (x->has_arg == required_argument && i < (argv->qty-1))
                {
                    set_required_argument(x, argv->entry[i+1]);
                    i++;
                }
                else if (x->has_arg == sticky_with_argument && i < (argv->qty-1))
                {
                    if (combine_opt && blength(combine) > 0)
                    {
                        add_multi_argument(combine, argv->entry[i]);
                        add_multi_argument(combine, argv->entry[i+1]);
                    }
                    else
                    {
                        set_required_argument(x, argv->entry[i+1]);
                    }
                    i++;
                }
                else if (x->has_arg == no_argument)
                {
                    set_required_argument(x, &btrue);
                }
                else if (x->has_arg == sticky_no_argument)
                {
                    if (combine_opt && blength(combine) > 0)
                    {
                        add_multi_argument(combine, argv->entry[i]);
                    }
                    else
                    {
                        set_required_argument(x, &btrue);
                    }
                }
                else if (x->has_arg == multi_argument && i < (argv->qty-1))
                {
                    if (blength(combine) > 0)
                    {
                        bstrListAdd(x->values, combine);
                        btrunc(combine, 0);
                        combine_opt = NULL;
                    }
                    //add_multi_argument(combine, argv->entry[i]);
                    combine_opt = x;
                }
                else if (combine_opt && combine_opt->has_arg == multi_argument && i < (argv->qty-1))
                {
                    add_multi_argument(combine, argv->entry[i]);
                }
                processed = 1;
                break;
            }
        }
        if (!processed && combine_opt)
        {
            add_multi_argument(combine, argv->entry[i]);
        }
    }
    if (blength(combine) > 0 && combine_opt)
    {
        bstrListAdd(combine_opt->values, combine);
    }
    bdestroy(combine);
    return 0;
}

void cliOptionsTitle(CliOptions* options, bstring title)
{
    if (options)
    {
        if (!options->title)
        {
            options->title = bstrcpy(title);
        }
        else
        {
            btrunc(options->title, 0);
            bconcat(options->title, title);
        }
    }
}

void cliOptionsProlog(CliOptions* options, bstring prolog)
{
    if (options)
    {
        if (!options->prolog)
        {
            options->prolog = bstrcpy(prolog);
        }
        else
        {
            btrunc(options->prolog, 0);
            bconcat(options->prolog, prolog);
        }
    }
}

void cliOptionsEpilog(CliOptions* options, bstring epilog)
{
    if (options)
    {
        if (!options->epilog)
        {
            options->epilog = bstrcpy(epilog);
        }
        else
        {
            btrunc(options->epilog, 0);
            bconcat(options->epilog, epilog);
        }
    }
}

double convertToSeconds(const_bstring input)
{
    double value = atof((char *)input->data);
    int len = blength(input);
    bstring unit;
    if (len >= 2 && input->data[len-2] == 'm' && input->data[len-1] == 's')
    {
        unit = bmidstr(input, blength(input) - 2, 2); // for 'ms'
    }
    else
    {
        unit = bmidstr(input, blength(input) - 1, 1); // for 's', 'm' and 'h'
    }
    struct tagbstring bms = bsStatic("ms");
    struct tagbstring bs = bsStatic("s");
    struct tagbstring bm = bsStatic("m");
    struct tagbstring bh = bsStatic("h");

    if (biseq(unit, &bms))
    {
        bdestroy(unit);
        return value * 0.001;
    }
    else if (biseq(unit, &bs))
    {
        bdestroy(unit);
        return value;
    }
    else if (biseq(unit, &bm))
    {
        bdestroy(unit);
        return value * 60;
    }
    else if (biseq(unit, &bh))
    {
        bdestroy(unit);
        return value * 3600;
    }
    else
    {
        bdestroy(unit);
        return value; // default to use 's' when no unit is mentioned for --runtime/-r 
    }
}

int assignBaseCliOptions(CliOptions* options, RuntimeConfig* runcfg)
{
    if ((!options) || (!runcfg))
    {
        return -EINVAL;
    }
    struct tagbstring bhelp = bsStatic("--help");
    struct tagbstring bverbose = bsStatic("--verbose");
    struct tagbstring btest = bsStatic("--test");
    struct tagbstring bfile = bsStatic("--file");
    struct tagbstring bkfolder = bsStatic("--kfolder");
    struct tagbstring btmpfolder = bsStatic("--tmpfolder");
    struct tagbstring barraysize = bsStatic("--arraysize");
    struct tagbstring biterations = bsStatic("--iterations");
    struct tagbstring bruntime = bsStatic("--runtime");
    struct tagbstring btrue = bsStatic("1");
    for (int i = 0; i < options->num_options; i++)
    {
        CliOption* opt = &options->options[i];
        if (bstrcmp(opt->name, &bkfolder) == BSTR_OK && blength(opt->value) > 0)
        {
            btrunc(runcfg->kernelfolder, 0);
            bconcat(runcfg->kernelfolder, opt->value);
            break;
        }
    }
    for (int i = 0; i < options->num_options; i++)
    {
        CliOption* opt = &options->options[i];
        if (bstrcmp(opt->name, &bhelp) == BSTR_OK && bstrcmp(opt->value, &btrue) == BSTR_OK)
        {
            runcfg->help = 1;
        }
        if (bstrcmp(opt->name, &bverbose) == BSTR_OK && blength(opt->value) > 0)
        {
            const char* cval = bdata(opt->value);
            runcfg->verbosity = atoi(cval);
        }
        if (bstrcmp(opt->name, &btest) == BSTR_OK && blength(opt->value) > 0)
        {
            bstring path = bformat("%s/%s.yaml", bdata(runcfg->kernelfolder), bdata(opt->value));
            const char* cpath = bdata(path);
            if (access(cpath, R_OK))
            {
                ERROR_PRINT(Test %s does not exist in folder %s, bdata(opt->value), bdata(runcfg->kernelfolder));
            }
            else
            {
                DEBUG_PRINT(DEBUGLEV_DETAIL, Using kernel file %s, bdata(path));
                btrunc(runcfg->testname, 0);
                bconcat(runcfg->testname, opt->value);
                btrunc(runcfg->pttfile, 0);
                bconcat(runcfg->pttfile, path);
            }
            bdestroy(path);
        }
        if (bstrcmp(opt->name, &bfile) == BSTR_OK && blength(opt->value) > 0)
        {
            btrunc(runcfg->pttfile, 0);
            bconcat(runcfg->pttfile, opt->value);
        }
        if (bstrcmp(opt->name, &btmpfolder) == BSTR_OK && blength(opt->value) > 0)
        {
            const char* cval = bdata(opt->value);
            if (access(cval, W_OK|X_OK))
            {
                ERROR_PRINT(Folder for temporary files not accessible or writable);
            }
            else
            {
                btrunc(runcfg->tmpfolder, 0);
                bconcat(runcfg->tmpfolder, opt->value);
            }
        }

        if (bstrcmp(opt->name, &barraysize) == BSTR_OK && blength(opt->value) > 0)
        {
            btrunc(runcfg->arraysize, 0);
            bconcat(runcfg->arraysize, opt->value);
        }

        if (bstrcmp(opt->name, &biterations) == BSTR_OK && blength(opt->value) > 0)
        {
            runcfg->iterations = atoi(bdata(opt->value));
        }

        if (bstrcmp(opt->name, &bruntime) == BSTR_OK && blength(opt->value) > 0)
        {
            runcfg->runtime =  convertToSeconds(opt->value);
            DEBUG_PRINT(DEBUGLEV_DETAIL, Runtime(in seconds) set as %.15f, runcfg->runtime);
        }

    }
    
    if (runcfg->runtime != -1.0 && runcfg->iterations != -1)
    {
	    ERROR_PRINT(Runtime and Iterations cannot be set at a time);
    	return -EINVAL;
    }
    
    return 0;
}

int generateTestCliOptions(CliOptions* options, RuntimeConfig* runcfg)
{
    int err = 0;
    if ((!options) || (!runcfg) || (!runcfg->tcfg))
    {
        return -EINVAL;
    }
    if (runcfg->tcfg->requirewg)
    {
        DEBUG_PRINT(DEBUGLEV_DETAIL, Workgroup is set to %s so -w/--workgroup option will be parsed, "true");
        err = addConstCliOptions(options, &wgroupopts);
        if (err < 0)
        {
            return err;
        }
    }
    else
    {
        DEBUG_PRINT(DEBUGLEV_DETAIL, Workgroup is set to %s so skipping -w/--workgroup parsing, "false");
    }

    for (int i = 0; i < runcfg->tcfg->num_params; i++)
    {
        TestConfigParameter* p = &runcfg->tcfg->params[i];
        CliOption opt = {
            .name = bformat("--%s", bdata(p->name)),
            .symbol = bformat("-%c", bchar(p->name, 0)),
            .has_arg = required_argument,
            .description = bstrcpy(p->description),
            .value = NULL,
            .values = NULL,
        };
        if (p->defvalue && blength(p->defvalue) > 0)
        {
            opt.value = bstrcpy(p->defvalue);
        }
        err = addCliOption(options, &opt);
        bdestroy(opt.name);
        bdestroy(opt.symbol);
        bdestroy(opt.description);
        if (opt.value) bdestroy(opt.value);
        if (err < 0)
        {
            return err;
        }
    }
    return 0;
}

static int add_runtime_parameter(RuntimeConfig* runcfg, TestConfigParameter* param, CliOption* opt)
{
    RuntimeParameterConfig* tmp = realloc(runcfg->params, (runcfg->num_params+1) * sizeof(RuntimeParameterConfig));
    if (!tmp)
    {
        return -ENOMEM;
    }
    runcfg->params = tmp;
    DEBUG_PRINT(DEBUGLEV_DETAIL, Adding runtime parameter %s, bdata(param->name));
    runcfg->params[runcfg->num_params].name = bstrcpy(param->name);
    runcfg->params[runcfg->num_params].value = NULL;
    runcfg->params[runcfg->num_params].values= NULL;
    if (opt->has_arg != multi_argument)
    {
        runcfg->params[runcfg->num_params].value = bstrcpy(opt->value);
    }
    else
    {
        runcfg->params[runcfg->num_params].values = bstrListCopy(opt->values);
    }
    runcfg->num_params++;
    return 0;
}

int assignTestCliOptions(CliOptions* options, RuntimeConfig* runcfg)
{
    int miss = 0;
    if ((!options) || (!runcfg))
    {
        return -EINVAL;
    }
    for (int i = 0; i < runcfg->tcfg->num_params; i++)
    {
        int found = 0;
        TestConfigParameter* param = &runcfg->tcfg->params[i];
        bstring paramname = bfromcstr("--");
        bconcat(paramname, param->name);
        DEBUG_PRINT(DEBUGLEV_DETAIL, Searching for parameter %s, bdata(paramname));
        for (int j = 0; j < options->num_options; j++)
        {
            CliOption* opt = &options->options[j];
            DEBUG_PRINT(DEBUGLEV_DETAIL, Comparing with CLI option %s, bdata(opt->name));
            if (bstrncmp(paramname, opt->name, blength(paramname)) == BSTR_OK)
            {
                int err = add_runtime_parameter(runcfg, param, opt);
                if (err != 0)
                {
                    ERROR_PRINT(Cannot add runtime parameter %s, bdata(opt->name));
                    continue;
                }
                found = 1;
            }
        }
        bdestroy(paramname);
        if (!found) miss++;
    }
    return miss;
}

/*int main(int argc, char* argv[])*/
/*{*/
/*    struct tagbstring bspace = bsStatic(" ");*/
/*    struct tagbstring bworkgroup = bsStatic("--workgroup");*/
/*    struct bstrList* args = bstrListCreate();*/
/*    for (int i = 1; i < argc; i++)*/
/*    {*/
/*        bstrListAddChar(args, argv[i]);*/
/*    }*/

/*    CliOptions options = {*/
/*        .num_options = 0,*/
/*        .options = NULL,*/
/*        .title = bfromcstr("likwid-bench"),*/
/*    };*/
/*    */
/*    addConstCliOptions(&options, &basecliopts);*/
/*    parseCliOptions(args, &options);*/
/*    printCliOptions(&options);*/
/*    printf("-----------------------------\n");*/
/*    for (int i = 0; i < options.num_options; i++)*/
/*    {*/
/*        CliOption* opt = &options.options[i];*/
/*        printf("Option %s\n", bdata(opt->name));*/
/*        if (bstrcmp(opt->name, &bworkgroup) == BSTR_OK)*/
/*        {*/
/*            for (int j = 0; j < opt->values->qty; j++)*/
/*            {*/
/*                printf("Workgroup: %s\n", bdata(opt->values->entry[j]));*/
/*                printf("-----------------------------\n");*/
/*                CliOptions wgroup_options = {*/
/*                    .num_options = 0,*/
/*                    .options = NULL,*/
/*                };*/
/*                struct bstrList* wargs = bsplit(opt->values->entry[j], ' ');*/
/*                addConstCliOptions(&wgroup_options, &wgroupopts);*/
/*                parseCliOptions(wargs, &wgroup_options);*/
/*                printCliOptions(&wgroup_options);*/
/*                destroyCliOptions(&wgroup_options);*/
/*                bstrListDestroy(wargs);*/
/*            }*/
/*        }*/
/*    }*/
/*    */
/*    */


/*    bstrListDestroy(args);*/
/*    destroyCliOptions(&options);*/
/*    return 0;*/
/*}*/
