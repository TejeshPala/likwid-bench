#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <error.h>
#include <bstrlib.h>
#include <bstrlib_helper.h>
#include <map.h>
#include <getopt_extra.h>

/*#ifndef global_verbosity*/
/*int global_verbosity = DEBUGLEV_DEVELOP;*/
/*#endif*/

static char* _getopt_extra_typestring(int return_flag)
{
    if (return_flag & RETURN_TYPE_MASK(RETURN_TYPE_LIST))
    {
        if (return_flag & RETURN_TYPE_MASK(RETURN_TYPE_BSTRING))
        {
            return "bstring list";
        }
        else if (return_flag & RETURN_TYPE_MASK(RETURN_TYPE_BOOL))
        {
            return "boolean list";
        }
        else if (return_flag & RETURN_TYPE_MASK(RETURN_TYPE_INT))
        {
            return "integer list";
        }
        else if (return_flag & RETURN_TYPE_MASK(RETURN_TYPE_UINT64))
        {
            return "uint64 list";
        }
        else if (return_flag & RETURN_TYPE_MASK(RETURN_TYPE_FLOAT))
        {
            return "float list";
        }
        else if (return_flag & RETURN_TYPE_MASK(RETURN_TYPE_DOUBLE))
        {
            return "double list";
        }
        // else if (return_flag & RETURN_TYPE_TIME_FLAG)
        // {
        //     return "time list";
        // }
        // else if (return_flag & RETURN_TYPE_BYTES_FLAG)
        // {
        //     return "bytes list";
        // }
        // else if (return_flag & RETURN_TYPE_FILE_FLAG)
        // {
        //     return "file list";
        // }
        else if (return_flag & RETURN_TYPE_MASK(RETURN_TYPE_STRING))
        {
            return "string list";
        }
        // else if (return_flag & RETURN_TYPE_DIR_FLAG)
        // {
        //     return "directory list";
        // }
    }
    else
    {
        if (return_flag & RETURN_TYPE_MASK(RETURN_TYPE_BSTRING))
        {
            return "bstring";
        }
        else if (return_flag & RETURN_TYPE_MASK(RETURN_TYPE_BOOL))
        {
            return "boolean";
        }
        else if (return_flag & RETURN_TYPE_MASK(RETURN_TYPE_INT))
        {
            return "integer";
        }
        else if (return_flag & RETURN_TYPE_MASK(RETURN_TYPE_UINT64))
        {
            return "uint64";
        }
        else if (return_flag & RETURN_TYPE_MASK(RETURN_TYPE_STRING))
        {
            return "string";
        }
        else if (return_flag & RETURN_TYPE_MASK(RETURN_TYPE_FLOAT))
        {
            return "float";
        }
        else if (return_flag & RETURN_TYPE_MASK(RETURN_TYPE_DOUBLE))
        {
            return "double";
        }
        // else if (return_flag & RETURN_TYPE_TIME_FLAG)
        // {
        //     return "time";
        // }
        // else if (return_flag & RETURN_TYPE_BYTES_FLAG)
        // {
        //     return "bytes";
        // }
        // else if (return_flag & RETURN_TYPE_FILE_FLAG)
        // {
        //     return "file";
        // }
        // else if (return_flag & RETURN_TYPE_DIR_FLAG)
        // {
        //     return "directory";
        // }
    }
    return "unknown";
}

#define COPY_OPTION(opt_in, opt_out) \
    (opt_out)->longname = (opt_in)->longname; \
    (opt_out)->shortname = (opt_in)->shortname; \
    (opt_out)->description = (opt_in)->description; \
    (opt_out)->return_flag = (opt_in)->return_flag; \
    (opt_out)->has_arg = (opt_in)->has_arg; \
    (opt_out)->flag = (opt_in)->flag; \
    (opt_out)->val = (opt_in)->val; \

int add_option(struct option_extra* option, struct option_extra** options)
{
    struct option_extra* opts = NULL;
    if ((!option) || (!options))
    {
        printf("Early exit\n");
        return -EINVAL;
    }
    opts = *options;
    if (opts == NULL)
    {
        DEBUG_PRINT(DEBUGLEV_DEVELOP, New option list);
        opts = malloc(2 * sizeof(struct option_extra));
        if (!opts)
        {
            return -ENOMEM;
        }
        memset(opts, 0, 2 * sizeof(struct option_extra));
        COPY_OPTION(option, &opts[0]);
        if (opts[0].return_flag == 0)
        {
            opts[0].return_flag = RETURN_TYPE_MASK(RETURN_TYPE_BOOL);
        }
        opts[1].longname = 0;
    }
    else
    {
        int num_opts = 0;
        while (opts[num_opts].longname != NULL && opts[num_opts].longname != 0 )
        {
            num_opts++;
        }
        DEBUG_PRINT(DEBUGLEV_DEVELOP, Resize option list to %d options, (num_opts+2));
        struct option_extra* tmp = realloc(opts, (num_opts+2) * sizeof(struct option_extra));
        if (tmp)
        {
            opts = tmp;
            DEBUG_PRINT(DEBUGLEV_DEVELOP, Copy new option to option list at %d, num_opts);
            COPY_OPTION(option, &opts[num_opts]);
            if (opts[num_opts].return_flag == 0)
            {
                opts[num_opts].return_flag = RETURN_TYPE_MASK(RETURN_TYPE_BOOL);
            }
            DEBUG_PRINT(DEBUGLEV_DEVELOP, Clean option %d in option list, num_opts+1);
            memset(&opts[num_opts+1], 0, sizeof(struct option));
            opts[num_opts+1].longname = 0;
        }
    }
    *options = opts;
    return 0;
}

int add_option_params(char* longname, int shortname, int return_flag, int has_arg, struct option_extra** options, int arg_flag)
{
    struct option_extra opt;
    opt.longname = longname;
    opt.shortname = shortname;
    opt.return_flag = return_flag;
    opt.arg_flag = arg_flag;
    opt.has_arg = has_arg;
    opt.flag = NULL;
    opt.description = NULL;
    opt.val = shortname;
    return add_option(&opt, options);
}

int del_option(char* longname, struct option_extra** options)
{
    if ((!longname) || (!options) || (!(*options)))
    {
        return -EINVAL;
    }
    struct option_extra* opts = *options;
    int num_opts = 0;
    int idx = -1;
    while (opts[num_opts].longname != NULL && opts[num_opts].longname != 0 )
    {
        if (strncmp(longname, opts[num_opts].longname, strlen(opts[num_opts].longname)) == 0)
        {
            idx = num_opts;
            DEBUG_PRINT(DEBUGLEV_DEVELOP, Option '%s' in option list at %d, longname, idx);
        }
        num_opts++;
    }
    if (idx >= 0 && idx < num_opts)
    {
        DEBUG_PRINT(DEBUGLEV_DEVELOP, Copy option %d to option list at %d, num_opts, idx);
        COPY_OPTION(&opts[num_opts-1], &opts[idx]);
        DEBUG_PRINT(DEBUGLEV_DEVELOP, Clean option %d in option list, num_opts-1);
        memset(&opts[num_opts-1], 0, sizeof(struct option));
        opts[num_opts-1].longname = 0;
    }
    else
    {
        DEBUG_PRINT(DEBUGLEV_DEVELOP, Option '%s' not in option list, longname);
    }
    return 0;
}

int add_option_list(struct option_extra* list, struct option_extra** out)
{
    int num_opts = 0;
    while (list[num_opts].longname != NULL && list[num_opts].longname != 0 )
    {
        add_option(&list[num_opts], out);
        num_opts++;
    }
    return num_opts;
}

void print_options(struct option_extra* options)
{
    int num_opts = 0;
    if (options)
    {
        while (options[num_opts].longname != NULL && options[num_opts].longname != 0 )
        {
            num_opts++;
        }
        for (int i = 0; i < num_opts; i++)
        {
            printf("%d\t-%c/--%s <%s> : %s\n", i, options[i].shortname, options[i].longname, _getopt_extra_typestring(options[i].return_flag), (options[i].description ? options[i].description : ""));
        }
    }
}

static int clioption_generate_getopt_strs(struct option_extra* options, bstring *shortopts, struct option** longopts)
{
    int i = 0, j = 0;
    int num_opts = 0;
    bstring sopts;
    struct option* lopts;
    if (!options)
    {
        return -EINVAL;
    }
    while (options[num_opts].longname != NULL && options[num_opts].longname != 0 )
    {
        num_opts++;
    }
    for (i = 0; i < num_opts; i++)
    {
        for (j = 0; j < num_opts; j++)
        {
            if (i == j) continue;
            if (options[i].shortname == options[j].shortname)
            {
                if (options[i].return_flag != options[j].return_flag)
                {
                    ERROR_PRINT(Option %d and %d use the short name '%c' but different type %x vs %x, i, j, options[i].shortname, options[i].return_flag, options[j].return_flag);
                    options[j].shortname = 0;
                }
                if (options[i].has_arg != options[j].has_arg)
                {
                    ERROR_PRINT(Option %d and %d use the short name '%c' but different has_arg %d vs %d, i, j, options[i].shortname, options[i].has_arg, options[j].has_arg);
                    options[j].shortname = 0;
                }
            }
            if (strncmp(options[i].longname, options[j].longname, strlen(options[i].longname)) == 0)
            {
                if (options[i].shortname != options[j].shortname)
                {
                    ERROR_PRINT(Option %d and %d use the long name '%s' but different short opt %c vs %c, i, j, options[i].longname, options[i].shortname, options[j].shortname);
                    options[j].shortname = options[i].shortname;
                }
            }
        }
    }
    lopts = malloc((num_opts+1) * sizeof(struct option_extra));
    if (!lopts)
    {
        return -ENOMEM;
    }
    sopts = bfromcstr("");
    for (i = 0; i < num_opts; i++)
    {
        lopts[i].name = options[i].longname;
        lopts[i].flag = options[i].flag;
        lopts[i].has_arg = options[i].has_arg;
        if (options[i].shortname)
        {
            lopts[i].val = options[i].shortname;
            bconchar(sopts, options[i].shortname);
            switch(options[i].has_arg)
            {
                case optional_argument:
                    bconchar(sopts, ':');
                case required_argument:
                    bconchar(sopts, ':');
                    break;
                default:
                    break;
            }
        }
    }
    memset(&lopts[num_opts], 0, sizeof(struct option_extra));
    *shortopts = sopts;
    *longopts = lopts;
    return num_opts;
}

static int _getopt_extra_add_bstrlist(struct option_extra_return* retopt, char* arg)
{
    if (retopt->qty == 0 && retopt->value.bstrlist == NULL)
    {
        DEBUG_PRINT(DEBUGLEV_DEVELOP, Create bstrList);
        retopt->value.bstrlist = bstrListCreate();
    }
    int ret = bstrListAddChar(retopt->value.bstrlist, arg);
    if (ret == 0)
    {
        DEBUG_PRINT(DEBUGLEV_DEVELOP, Adding %s to bstrList (%d), arg, retopt->value.bstrlist->qty);
        retopt->qty = retopt->value.bstrlist->qty;
    }
    return 0;
}

static int _getopt_extra_add_intlist(struct option_extra_return* retopt, char* arg)
{
    if (retopt->qty == 0)
        retopt->value.intlist = NULL;

    int* tmp = realloc(retopt->value.intlist, (retopt->qty+1) * sizeof(int));
    if (tmp)
    {
        retopt->value.intlist = tmp;
        retopt->value.intlist[retopt->qty] = (arg != NULL ? atoi(arg) : 0);
        retopt->qty++;
        DEBUG_PRINT(DEBUGLEV_DEVELOP, Adding %d to intList (%d), retopt->value.intlist[retopt->qty-1], retopt->qty);
    }
    return 0;
}

static int _getopt_extra_add_floatlist(struct option_extra_return* retopt, char* arg)
{
    if (retopt->qty == 0)
        retopt->value.floatlist = NULL;

    float* tmp = realloc(retopt->value.floatlist, (retopt->qty+1) * sizeof(float));
    if (tmp)
    {
        retopt->value.floatlist = tmp;
        retopt->value.floatlist[retopt->qty] = (arg != NULL ? atof(arg) : 0);
        retopt->qty++;
        DEBUG_PRINT(DEBUGLEV_DEVELOP, Adding %f to floatList (%d), retopt->value.floatlist[retopt->qty-1], retopt->qty);
    }
    return 0;
}

static int _getopt_extra_add_doublelist(struct option_extra_return* retopt, char* arg)
{
    if (retopt->qty == 0)
        retopt->value.doublelist = NULL;

    double* tmp = realloc(retopt->value.doublelist, (retopt->qty+1) * sizeof(double));
    if (tmp)
    {
        retopt->value.doublelist = tmp;
        retopt->value.doublelist[retopt->qty] = (arg != NULL ? strtod(arg, NULL) : 0);
        retopt->qty++;
        DEBUG_PRINT(DEBUGLEV_DEVELOP, Adding %f to doubleList (%d), retopt->value.doublelist[retopt->qty-1], retopt->qty);
    }
    return 0;
}

static int _getopt_long_extra_addlist(struct option_extra_return* retopt, char* arg)
{
    int ret = -EINVAL;
    if (retopt->return_flag & ARG_FLAG_MASK(ARG_FLAG_FILE))
    {
        if (retopt->return_flag & RETURN_TYPE_MASK(RETURN_TYPE_BSTRING) || retopt->return_flag & RETURN_TYPE_MASK(RETURN_TYPE_STRING))
        {
            if (access(arg, F_OK))
            {
                return -ENOENT;
            }
            DEBUG_PRINT(DEBUGLEV_DEVELOP, Arg '%s' is a file, arg);
        }
    }
    if (retopt->return_flag & ARG_FLAG_MASK(ARG_FLAG_DIR))
    {
        if (retopt->return_flag & RETURN_TYPE_MASK(RETURN_TYPE_BSTRING) || retopt->return_flag & RETURN_TYPE_MASK(RETURN_TYPE_STRING))
        {
            struct stat statbuf;
            if (stat(arg, &statbuf) == 0)
            {
                if (!(S_ISDIR(statbuf.st_mode)))
                {
                    return -ENOENT;
                }
                DEBUG_PRINT(DEBUGLEV_DEVELOP, Arg '%s' is a directory, arg);
            }
        }
    }
    if (retopt->return_flag & RETURN_TYPE_MASK(RETURN_TYPE_BSTRING))
    {
        ret = _getopt_extra_add_bstrlist(retopt, optarg);
    }
    else if (retopt->return_flag & RETURN_TYPE_MASK(RETURN_TYPE_FLOAT))
    {
        ret = _getopt_extra_add_floatlist(retopt, optarg);
    }
    else if (retopt->return_flag & RETURN_TYPE_MASK(RETURN_TYPE_DOUBLE))
    {
        ret = _getopt_extra_add_doublelist(retopt, optarg);
    }
/*    else if (return_flag & RETURN_TYPE_STRING_FLAG)*/
/*    {*/
/*        ret = _getopt_extra_add_strlist(retopt, optarg);*/
/*    }*/
    else if (retopt->return_flag & RETURN_TYPE_MASK(RETURN_TYPE_INT))
    {
        ret = _getopt_extra_add_intlist(retopt, optarg);
    }
/*    else if (return_flag & RETURN_TYPE_BYTES_FLAG)*/
/*    {*/
/*        ret = _getopt_extra_add_byteslist(retopt, optarg);*/
/*    }*/
/*    else if (return_flag & RETURN_TYPE_TIME_FLAG)*/
/*    {*/
/*        ret = _getopt_extra_add_timelist(retopt, optarg);*/
/*    }*/
    return ret;
}

static int _getopt_extra_set_bstr(struct option_extra_return* retopt, char* arg)
{
    retopt->value.bstrvalue = (arg != NULL ? bfromcstr(arg) : bfromcstr(""));
    DEBUG_PRINT(DEBUGLEV_DEVELOP, Setting bstring '%s', bdata(retopt->value.bstrvalue));
    return 0;
}

static int _getopt_extra_set_int(struct option_extra_return* retopt, char* arg)
{
    retopt->value.intvalue = (arg != NULL ? atoi(arg) : 0);
    DEBUG_PRINT(DEBUGLEV_DEVELOP, Setting integer '%d', retopt->value.intvalue);
    return 0;
}

static int _getopt_extra_set_uint64(struct option_extra_return* retopt, char* arg)
{
    retopt->value.uint64value = (arg != NULL ? strtoll(arg, NULL, 10) : 0);
    DEBUG_PRINT(DEBUGLEV_DEVELOP, Setting uint64 '%ld', retopt->value.uint64value);
    return 0;
}

static int _getopt_extra_set_float(struct option_extra_return* retopt, char* arg)
{
    retopt->value.floatvalue = (arg != NULL ? atof(arg) : 0);
    DEBUG_PRINT(DEBUGLEV_DEVELOP, Setting float '%f', retopt->value.floatvalue);
    return 0;
}

static int _getopt_extra_set_double(struct option_extra_return* retopt, char* arg)
{
    retopt->value.doublevalue = (arg != NULL ? strtod(arg, NULL) : 0);
    DEBUG_PRINT(DEBUGLEV_DEVELOP, Setting double '%f', retopt->value.doublevalue);
    return 0;
}

static int _getopt_extra_set_str(struct option_extra_return* retopt, char* arg)
{
    int slen = strlen(arg);
    if (slen > 0)
    {
        retopt->value.strvalue = malloc((slen+1) * sizeof(char));
        if (retopt->value.strvalue)
        {
            int ret = snprintf(retopt->value.strvalue, slen, "%s", arg);
            if (ret > 0)
            {
                retopt->value.strvalue[ret] = '\0';
            }
        }
    }
    DEBUG_PRINT(DEBUGLEV_DEVELOP, Setting string '%s', retopt->value.strvalue);
    return 0;
}

static int __getopt_extra_parse_time(char* arg, double* time)
{
    double value = 0;
    char suffix[10];
    int ret = sscanf(arg, "%lf%9s", &value, (char *)suffix);
    if (ret == 1 && value > 0)
    {
        *time = value;
    }
    ret = -EINVAL;
    if (suffix[0] == 's')
    {
        *time = value;
        ret = 0;
    }
    else if (suffix[0] == 'm' && suffix[1] == 's')
    {
        *time = value*1E-3;
        ret = 0;
    }
    return ret;
}

static int __getopt_extra_parse_bytes(char* arg, uint64_t* bytes)
{
    uint64_t value = 0;
    char suffix[10];
    int ret = sscanf(arg, "%ld%9s", &value, (char *)suffix);
    if (ret < 1 || ret > 2)
    {
        return -EINVAL;
    }
    if (suffix)
    {
        if ((suffix[0] == 'k' && suffix[1] == 'B') ||
            (suffix[0] == 'K' && suffix[1] == 'B'))
        {
            *bytes = value*1000;
        }
        else if ((suffix[0] == 'k' && suffix[1] == 'i' && suffix[2] == 'B') ||
                 (suffix[0] == 'K' && suffix[1] == 'i' && suffix[2] == 'B'))
        {
            *bytes = value*1024;
        }
        else if (suffix[0] == 'M' && suffix[1] == 'B')
        {
            *bytes = value*1000*1000;
        }
        else if (suffix[0] == 'M' && suffix[1] == 'i' && suffix[2] == 'B')
        {
            *bytes = value*1024*1024;
        }
        else if (suffix[0] == 'G' && suffix[1] == 'B')
        {
            *bytes = value*1000*1000*1000;
        }
        else if (suffix[0] == 'G' && suffix[1] == 'i' && suffix[2] == 'B')
        {
            *bytes = value*1024*1024*1024;
        }
        else if (suffix[0] == 'T' && suffix[1] == 'B')
        {
            *bytes = value*1000*1000*1000*1000;
        }
        else if (suffix[0] == 'T' && suffix[1] == 'i' && suffix[2] == 'B')
        {
            *bytes = value*1024*1024*1024*1024;
        }
        else if (suffix[0] == 'B')
        {
            *bytes = value;
        }
        else
        {
            return -EINVAL;
        }
    }
    else if (value > 0)
    {
        *bytes = value;
    }
    return 0;
}

static int _getopt_extra_set_time(struct option_extra_return* retopt, char* arg)
{
    if (arg)
    {
        int ret = __getopt_extra_parse_time(arg, &retopt->value.doublevalue);
        DEBUG_PRINT(DEBUGLEV_DEVELOP, Setting time '%f', retopt->value.doublevalue);
        return ret;
    }
    return -EINVAL;
}

static int _getopt_extra_set_bytes(struct option_extra_return* retopt, char* arg)
{
    if (arg)
    {
        int ret = __getopt_extra_parse_bytes(arg, &retopt->value.uint64value);
        DEBUG_PRINT(DEBUGLEV_DEVELOP, Setting bytes '%ld', retopt->value.uint64value);
        return ret;
    }
    return -EINVAL;
}

static int _getopt_long_extra_setvalue(struct option_extra_return* retopt, char* arg)
{
    int ret = -EINVAL;
    if (retopt->arg_flag & ARG_FLAG_MASK(ARG_FLAG_FILE))
    {
        if (retopt->return_flag & RETURN_TYPE_MASK(RETURN_TYPE_BSTRING) || retopt->return_flag & RETURN_TYPE_MASK(RETURN_TYPE_STRING))
        {
            if (access(arg, F_OK))
            {
                return -ENOENT;
            }
            DEBUG_PRINT(DEBUGLEV_DEVELOP, Arg '%s' is a file, arg);
        }
    }
    if (retopt->arg_flag & ARG_FLAG_MASK(ARG_FLAG_DIR))
    {
        if (retopt->return_flag & RETURN_TYPE_MASK(RETURN_TYPE_BSTRING) || retopt->return_flag & RETURN_TYPE_MASK(RETURN_TYPE_STRING))
        {
            struct stat statbuf;
            if (stat(arg, &statbuf) == 0)
            {
                if (!(S_ISDIR(statbuf.st_mode)))
                {
                    return -ENOENT;
                }
                DEBUG_PRINT(DEBUGLEV_DEVELOP, Arg '%s' is a directory, arg);
            }
        }
    }
    if (retopt->return_flag & RETURN_TYPE_MASK(RETURN_TYPE_BSTRING))
    {
        ret = _getopt_extra_set_bstr(retopt, optarg);
    }
    else if (retopt->return_flag & RETURN_TYPE_MASK(RETURN_TYPE_BOOL))
    {
        retopt->value.boolvalue = 1;
        DEBUG_PRINT(DEBUGLEV_DEVELOP, Setting boolean '%d', retopt->value.boolvalue);
        ret = 0;
    }
    else if (retopt->return_flag & RETURN_TYPE_MASK(RETURN_TYPE_FLOAT))
    {
        ret = _getopt_extra_set_float(retopt, optarg);
    }
    else if (retopt->return_flag & RETURN_TYPE_MASK(RETURN_TYPE_DOUBLE))
    {
        ret = _getopt_extra_set_double(retopt, optarg);
    }
    else if (retopt->return_flag & RETURN_TYPE_MASK(RETURN_TYPE_STRING))
    {
        ret = _getopt_extra_set_str(retopt, optarg);
    }
    else if (retopt->return_flag & RETURN_TYPE_MASK(RETURN_TYPE_INT))
    {
        ret = _getopt_extra_set_int(retopt, optarg);
    }
    else if (retopt->return_flag & RETURN_TYPE_MASK(RETURN_TYPE_UINT64))
    {
        ret = _getopt_extra_set_uint64(retopt, optarg);
    }
    // else if (retopt->return_flag & RETURN_TYPE_BYTES_FLAG)
    // {
    //     ret = _getopt_extra_set_bytes(retopt, optarg);
    // }
    // else if (retopt->return_flag & RETURN_TYPE_TIME_FLAG)
    // {
    //     ret = _getopt_extra_set_time(retopt, optarg);
    // }
    return ret;
}

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

void free_returns(void* value)
{
    struct option_extra_return* retopt = (struct option_extra_return*) value;
    if (retopt->return_flag & RETURN_TYPE_MASK(RETURN_TYPE_LIST))
    {
        if (retopt->return_flag & RETURN_TYPE_MASK(RETURN_TYPE_BSTRING))
        {
            DEBUG_PRINT(DEBUGLEV_DEVELOP, Destroying bstring list);
            bstrListDestroy(retopt->value.bstrlist);
        }
        else if (retopt->return_flag & RETURN_TYPE_MASK(RETURN_TYPE_INT))
        {
            DEBUG_PRINT(DEBUGLEV_DEVELOP, Destroying integer list);
            free(retopt->value.intlist);
        }
        retopt->qty = 0;
    }
    else
    {
        if (retopt->return_flag & RETURN_TYPE_MASK(RETURN_TYPE_BSTRING))
        {
            DEBUG_PRINT(DEBUGLEV_DEVELOP, Destroying bstring value);
            bdestroy(retopt->value.bstrvalue);
        }
    }
    free(retopt);
}

void print_options_normal(struct option* options)
{
    int i = 0;
    while (options[i].name != 0)
    {
        printf("--%s, %d, %c\n", options[i].name, options[i].has_arg, options[i].val);
        i++;
    }
}

int __check_flags_and_types(struct option_extra* options) {
    int ret = 0;
    int i = 0;
    while (options[i].longname != 0)
    {
        int count_type = 0;
        for (int j = RETURN_TYPE_MIN; j <= RETURN_TYPE_DOUBLE; j++)
        {
            if (options[i].return_flag & RETURN_TYPE_MASK(j))
                count_type++;
        }
        int count_flags = 0;
        for (int j = ARG_FLAG_MIN; j < ARG_FLAG_MAX; j++)
        {
            if (options[i].arg_flag & ARG_FLAG_MASK(j))
                count_flags++;
        }
        DEBUG_PRINT(DEBUGLEV_DEVELOP, Option --%s with %d arg flags and %d return types, options[i].longname, count_flags, count_type)
        if (count_type > 1)
        {
            ERROR_PRINT(Option --%s uses multiple return types, options[i].longname);
            ret = -1;
        }
        if (count_flags > 1)
        {
            ERROR_PRINT(Option --%s uses multiple arg flags, options[i].longname);
            ret = -1;
        }
        if ((options[i].arg_flag & ARG_FLAG_MASK(ARG_FLAG_FILE) || options[i].arg_flag & ARG_FLAG_MASK(ARG_FLAG_DIR))   &&
            (!(options[i].return_flag & RETURN_TYPE_MASK(RETURN_TYPE_BSTRING)|| options[i].return_flag & RETURN_TYPE_MASK(RETURN_TYPE_STRING))))
        {
            ERROR_PRINT(Option --%s uses FILE or DIR flag but return type is not a string type, options[i].longname);
            ret = -1;
        }
        if (ret < 0)
            break;
        i++;
    }
    return ret;
}

int getopt_long_extra(int argc, char* argv[], char* shortopts, struct option_extra* options, Map_t *optionmap)
{
    int ret = 0;
    int c;
    int option_index = -1;
    int return_flag = RETURN_TYPE_BOOL;
    int i = 0;
    int has_long = 0;
    ret = __check_flags_and_types(options);
    if (ret < 0)
    {
        return -1;
    }
    bstring sopts = (shortopts != NULL ? bfromcstr(shortopts) : bfromcstr(""));
    struct option* lopts = NULL;
    bstring sopts_new;
    int num_opts = clioption_generate_getopt_strs(options, &sopts_new, &lopts);
    bconcat(sopts, sopts_new);
    bdestroy(sopts_new);
    Map_t climap;
    char** argv_copy = NULL;
    int argc_copy = 0;
    init_smap(&climap, free_returns);

    optind = 0;
    opterr = 0;
    optopt = 0;
    optarg = NULL;
    argc_copy = copy_cli(argc, argv, &argv_copy);
    for (i = 0; i < argc; i++)
    {
        printf("%s ", argv[i]);
    }
    printf("\n");
    char* sopt_char = bdata(sopts);
    printf("Shortopts %s\n", sopt_char);
    print_options_normal(lopts);
    while ((c = getopt_long (argc_copy, argv_copy, sopt_char, lopts, &option_index)) != -1)
    {
        switch (c)
        {
            case '?':
                //printf("Error %c %d %s\n", optopt, optind, optarg);
                for (i = 0; i < num_opts; i++)
                {
                    if (lopts[i].val == optopt)
                    {
                        if (lopts[i].has_arg == required_argument)
                        {
                            printf("Option -%c/--%s requires an argument\n", optopt, lopts[i].name);
                        }
                        else
                        {
                            printf("Error with option -%c\n", optopt);
                        }
                    }
                }
                break;
            case 0:
                printf("xlong %s = %s (type %s)\n", lopts[option_index].name, optarg, _getopt_extra_typestring(options[i].return_flag));
                break;
            default:
                has_long = -1;
                char* key =  NULL;
                for (i = 0; i < num_opts; i++)
                {
                    if (lopts[i].val == c)
                    {
                        printf("long %s = %s (type %s)\n", lopts[i].name, optarg, _getopt_extra_typestring(options[i].return_flag));
                        has_long = i;
                        return_flag = options[i].return_flag;
                        key = (char*)lopts[i].name;
                        break;
                    }
                }
                if (has_long < 0)
                {
                    printf("short %c = %s (type %s)\n", c, optarg, _getopt_extra_typestring(options[i].return_flag));
                    key = (char*)&c;
                }
                struct option_extra_return* retopt = NULL;
                ret = get_smap_by_key(climap, (char*)key, (void**)&retopt);
                DEBUG_PRINT(DEBUGLEV_DEVELOP, Lookup %s in map returns %d, key, ret);
                if (ret == 0)
                {
                    DEBUG_PRINT(DEBUGLEV_DEVELOP, Key %s exists, key);
                    if (retopt->return_flag == return_flag)
                    {
                        if (return_flag & RETURN_TYPE_MASK(RETURN_TYPE_LIST))
                        {
                            ret = _getopt_long_extra_addlist(retopt, optarg);
                        }
                        else
                        {
                            ret = _getopt_long_extra_setvalue(retopt, optarg);
                        }
                    }
                }
                else
                {
                    DEBUG_PRINT(DEBUGLEV_DEVELOP, New key %s, key);
                    retopt = malloc(sizeof(struct option_extra_return));
                    if (retopt)
                    {
                        memset(retopt, 0, sizeof(struct option_extra_return));
                        retopt->return_flag = return_flag;
                        retopt->qty = 0;
                        if (return_flag & RETURN_TYPE_MASK(RETURN_TYPE_LIST))
                        {
                            ret = _getopt_long_extra_addlist(retopt, optarg);
                        }
                        else
                        {
                            ret = _getopt_long_extra_setvalue(retopt, optarg);
                        }
                        ret = add_smap(climap, (char*)key, (void*)retopt);
                        DEBUG_PRINT(DEBUGLEV_DEVELOP, Extending map at index %d, ret);
                        if (ret >= 0) ret = 0;
                    }
                    else
                    {
                        ret = -ENOMEM;
                    }
                }
                break;
        }
    }
    free(lopts);
    bdestroy(sopts);
    free_cli(argc_copy, argv_copy);
    if (climap)
    {
/*        DEBUG_PRINT(DEBUGLEV_DEVELOP, Destroying map);*/
/*        destroy_smap(climap);*/
        *optionmap = climap;
    }
    return ret;
}
