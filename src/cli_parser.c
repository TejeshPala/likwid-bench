#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "cli_parser.h"
#include "bstrlib.h"
#include "bstrlib_helper.h"
#include "test_types.h"

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
    if ((!options) || (!new))
    {
        return -EINVAL;
    }
    for (int i = 0; i < options->num_options; i++)
    {
        CliOption* x = &options->options[i];
        if (bstrcmp(x->name, new->name) == BSTR_OK)
        {
            name_free = 0;
        }
        if (bstrcmp(x->symbol, new->symbol) == BSTR_OK)
        {
            symbol_free = 0;
        }
    }
    if ((!name_free) && (!symbol_free))
    {
        printf("Name '%s' and symbol '%s' already taken\n", bdata(new->name), bdata(new->symbol));
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
        x->symbol = bfromcstr("****");
    }
    x->has_arg = new->has_arg;
    x->description = bstrcpy(new->description);
    x->value = bfromcstr("");
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
                    add_multi_argument(combine, argv->entry[i]);
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

int assignBaseCliOptions(CliOptions* options, RuntimeConfig* runcfg)
{
    if ((!options) || (!runcfg))
    {
        return -EINVAL;
    }
    struct tagbstring bhelp = bsStatic("--help");
    struct tagbstring bverbose = bsStatic("--verbose");
    struct tagbstring btrue = bsStatic("1");
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
    }
    return 0;
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
