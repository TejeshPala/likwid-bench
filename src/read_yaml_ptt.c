#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>

#include "bstrlib.h"
#include "bstrlib_helper.h"
#include "map.h"

#include "read_yaml_ptt.h"
#include "test_types.h"
#include "error.h"

struct tagbstring byaml_end = bsStatic("...");
struct tagbstring byaml_begin = bsStatic("---");
struct tagbstring bnewline = bsStatic("\n");

static int read_indent(bstring str)
{
    int i = 0;
    while (i < blength(str) && bchar(str, i) == ' ') { i++; }
    return i;
}

static int read_valueindent(bstring str)
{
    int i = 0;
    int colon = bstrchrp(str, ':', 0);
    if (colon >= 0)
        i = colon+1;
    while (i < blength(str) && bchar(str, i) == ' ') { i++; }
    return i;
}

static int read_keyvalue(bstring str, bstring *key, bstring *value)
{
    bstring k = NULL, v = NULL;
    int colon = bstrchrp(str, ':', 0);
    int dash = bstrchrp(str, '-', 0);
    int indent = read_indent(str);
    int valind = read_valueindent(str);
    if (colon == BSTR_ERR && dash == BSTR_ERR)
    {
        printf("EarlyError\n");
        return -1;
    }
    //printf("Indent %d ValIndent %d dash %d colon %d\n", indent, valind, dash, colon);
    int keystart = (dash >= 0 ? dash+1 : indent);
    int keyend = (colon >= 0 ? colon : blength(str));
    int valstart = valind;
    int valend = (colon >= 0 ? blength(str) : -1);
    if (keystart > keyend)
    {
        keystart = indent;
    }
    //printf("kstart %d kend %d vstart %d vend %d\n", keystart, keyend, valstart, valend);
    k = bmidstr(str, keystart, keyend-keystart);
    btrimws(k);
    *key = k;
    //printf("Key %s\n", bdata(k));
    if (value)
    {
        if (valstart > 0)
        {
            v = bmidstr(str, valstart, valend-valstart);
            //btrimws(v);
            //printf("Key '%s' Value '%s'\n\n", bdata(k), bdata(v));
        }
        else
        {
            //printf("KeyOnly '%s'\n\n", bdata(k));
            v = bfromcstr("");
        }
        *value = v;
    }

    return 0;
}

static bstring read_code(bstring bptt)
{
    int i = 0;
    int start_copy = 0;
    bstring basm = bfromcstr("");

    struct bstrList* lines = bsplit(bptt, '\n');
    
    for (i = 0; i < lines->qty; i++)
    {
        if (bstrnicmp(lines->entry[i], &byaml_end, 3) == BSTR_OK)
        {
            start_copy = 1;
            continue;
        }
        if (start_copy)
        {
            bconcat(basm, lines->entry[i]);
            bconcat(basm, &bnewline);
        }
    }
    bstrListDestroy(lines);
    return basm;
}


static int read_obj(bstring bptt, struct bstrList** objects)
{
    int i = 0, j = 0;
    int ind_off = -1;
    int ind_start = -1;

    struct bstrList* objs = bstrListCreate();

    struct bstrList* lines = bsplit(bptt, '\n');
    for (i = 0; i < lines->qty; i++)
    {
        if (bstrnicmp(lines->entry[i], &byaml_begin, 3) == BSTR_OK || blength(lines->entry[i]) == 0)
        {
            continue;
        }
        else if (bstrnicmp(lines->entry[i], &byaml_end, 3) == BSTR_OK)
        {
            break;
        }
        int indent = read_indent(lines->entry[i]);
        if (indent >= 0)
        {
            if (ind_off == -1)
            {
                ind_off = indent;
            }
            if (ind_start == -1)
            {
                ind_start = i;
            }
            else if (ind_start >= 0 && indent == ind_off)
            {
                //printf("ind_start %d ind_off %d indent %d\n", ind_start, ind_off, indent);
                bstring t = bfromcstr("");
                for (j = ind_start; j < i; j++)
                {
                    if (blength(lines->entry[j]) > 0)
                    {
                        bconcat(t, lines->entry[j]);
                        //btrimws(t);
                        bconcat(t, &bnewline);
                    }
                }
                bstrListAdd(objs, t);
                bdestroy(t);
                ind_start = i;
            }
        }
    }
    if (ind_start >= 0)
    {
        //printf("ind_start %d ind_off %d\n", ind_start, ind_off);
        bstring t = bfromcstr("");
        for (j = ind_start; j < i; j++)
        {
            bconcat(t, lines->entry[j]);
            bconcat(t, &bnewline);
        }
        bstrListAdd(objs, t);
        bdestroy(t);
    }
    bstrListDestroy(lines);
    *objects = objs;
    return 0;
}

static int read_yaml_ptt_list(bstring obj, struct bstrList** list)
{
    int count = 0;
    if (blength(obj) > 0 && list && *list)
    {
        struct bstrList* objlist = NULL;
        read_obj(obj, &objlist);
        for (int l = 0; l < objlist->qty; l++)
        {
            btrimws(objlist->entry[l]);
            bstring x;
            int ret = read_keyvalue(objlist->entry[l], &x, NULL);
            btrimws(x);
            if (blength(x) > 0) bstrListAdd(*list, x);
            bdestroy(x);
            count++;
        }
        bstrListDestroy(objlist);
    }
    return count;
}

static int read_yaml_ptt_dict(bstring obj, TestConfigVariable** list)
{
    int count = 0;
    TestConfigVariable* tmp = NULL;
    if (blength(obj) > 0 && list)
    {
        struct bstrList* objlist = NULL;
        read_obj(obj, &objlist);
        tmp = malloc(objlist->qty * sizeof(TestConfigVariable));
        if (tmp)
        {
            for (int l = 0; l < objlist->qty; l++)
            {
                TestConfigVariable* vcv = &tmp[l];
                bstring vk, vv;
                int ret = read_keyvalue(objlist->entry[l], &vcv->name, &vcv->value);
                btrimws(vcv->value);
                count++;
            }
        }
        bstrListDestroy(objlist);
        *list = tmp;
    }
    return count;
}


int read_yaml_ptt(char* filename, TestConfig_t* config)
{
    int i = 0, j = 0;
    int ret = 0;
    if (access(filename, R_OK))
    {
        return -EACCES;
    }
    int (*myatoi)(const char *nptr) = &atoi;
    struct bstrList* objs, *streams = NULL;
    struct tagbstring bstr = bsStatic("Streams");
    struct tagbstring bstrdatatype = bsStatic("datatype");
    struct tagbstring bstrdatatypedbl = bsStatic("double");
    struct tagbstring bstrdatatypesgl = bsStatic("single");
    struct tagbstring bstrdatatypeint = bsStatic("integer");
    struct tagbstring bstrdimensions = bsStatic("dimensions");
    struct tagbstring bstrdimsizes = bsStatic("dimsizes");
    struct tagbstring bstropts = bsStatic("options");
    struct tagbstring bstrinit = bsStatic("initialization");
    struct tagbstring bname = bsStatic("Name");
    struct tagbstring bdesc = bsStatic("Description");
    struct tagbstring blang = bsStatic("Language");
    struct tagbstring bconstants = bsStatic("Constants");
    struct tagbstring bvars = bsStatic("Variables");
    struct tagbstring bmetrics = bsStatic("Metrics");
    struct tagbstring bparams = bsStatic("Parameters");
    struct tagbstring bparamopts = bsStatic("options");
    struct tagbstring bparamdef = bsStatic("default");
    struct tagbstring bparamdesc = bsStatic("description");
    struct tagbstring bthreads = bsStatic("Threads");
    struct tagbstring bthreadsoff = bsStatic("offsets");
    struct tagbstring bthreadssize = bsStatic("sizes");
    struct tagbstring bflags = bsStatic("FeatureFlag");
    struct tagbstring brequirewg = bsStatic("RequireWorkgroup");
    struct tagbstring btrue = bsStatic("true");
    struct tagbstring bperthread = bsStatic("perthread");
    struct tagbstring brand = bsStatic("rand");
    bstring bptt = read_file(filename);
    if (blength(bptt) == 0)
    {
        return -EACCES;
    }
    TestConfig_t conf = malloc(sizeof(TestConfig));
    if (!conf)
    {
        return -ENOMEM;
    }
    memset(conf, 0, sizeof(TestConfig));
    conf->code = NULL;
    conf->flags = bstrListCreate();
    conf->requirewg = false;
    conf->initialization = false;
    read_obj(bptt, &objs);
    for (i = 0; i < objs->qty; i++)
    {
        bstring k, v;
        ret = read_keyvalue(objs->entry[i], &k, &v);
        if (ret == 0)
        {
            if (bstrnicmp(k, &bstr, blength(&bstr)) == BSTR_OK)
            {
                //printf("READ_OBJ STREAMS\n");
                read_obj(v, &streams);
                conf->streams = malloc(streams->qty * sizeof(TestConfigStream));
                if (conf->streams)
                {
                    for (j = 0; j < streams->qty; j++)
                    {
                        TestConfigStream* s = &conf->streams[j];
                        s->btype = NULL;
                        s->name = NULL;
                        s->dims = bstrListCreate();
                        bstring sv;
                        ret = read_keyvalue(streams->entry[j], &s->name, &sv);
                        if (ret == 0)
                        {
                            //printf("Stream '%s'\n", bdata(s->name));
                            
                            struct bstrList* vars = NULL;
                            read_obj(sv, &vars);
                            for (int l = 0; l < vars->qty; l++)
                            {
                                bstring vk, vv;
                                ret = read_keyvalue(vars->entry[l], &vk, &vv);
                                if (ret == 0)
                                {
                                    if (bstrnicmp(vk, &bstrdimensions, blength(&bstrdimensions)) == BSTR_OK)
                                    {
                                        s->num_dims = myatoi(bdata(vv));
                                        //printf("num_dims %d\n", s->num_dims);
                                    }
                                    else if (bstrnicmp(vk, &bstrdatatype, blength(&bstrdatatype)) == BSTR_OK)
                                    {
                                        if (bstrnicmp(vv, &bstrdatatypedbl, blength(&bstrdatatypedbl)) == BSTR_OK)
                                        {
                                            s->btype = bstrcpy(vv);
                                            btrimws(s->btype);
                                            s->type = TEST_STREAM_TYPE_DOUBLE;
                                        }
                                        else if (bstrnicmp(vv, &bstrdatatypesgl, blength(&bstrdatatypesgl)) == BSTR_OK)
                                        {
                                            s->btype = bstrcpy(vv);
                                            btrimws(s->btype);
                                            s->type = TEST_STREAM_TYPE_SINGLE;
                                        }
                                        else if (bstrnicmp(vv, &bstrdatatypeint, blength(&bstrdatatypeint)) == BSTR_OK)
                                        {
                                            s->btype = bstrcpy(vv);
                                            btrimws(s->btype);
                                            s->type = TEST_STREAM_TYPE_INT;
                                        }
                                        else
                                        {
                                            printf("Unknown stream type '%s'\n", bdata(vv));
                                        }
                                    }
                                    else if (bstrnicmp(vk, &bstrinit, blength(&bstrinit)) == BSTR_OK)
                                    {
                                        btrimws(vv);
                                        srand(time(NULL));
                                        if (s->type = TEST_STREAM_TYPE_SINGLE)
                                        {
                                            if (bstrnicmp(vv, &brand, blength(&brand)) == BSTR_OK && blength(vv) == blength(&brand))
                                            {
                                                s->data.fval = (float)rand() / (float)RAND_MAX;
                                            }
                                            else
                                            {
                                                batof(vv, &s->data.fval);
                                            }
                                        }
                                        else if (s->type = TEST_STREAM_TYPE_DOUBLE)
                                        {
                                            if (bstrnicmp(vv, &brand, blength(&brand)) == BSTR_OK && blength(vv) == blength(&brand))
                                            {
                                                s->data.dval = (double)rand() / (double)RAND_MAX;
                                            }
                                            else
                                            {
                                                batod(vv, &s->data.dval);
                                            }
                                        }
                                        else if (s->type = TEST_STREAM_TYPE_INT)
                                        {
                                            if (bstrnicmp(vv, &brand, blength(&brand)) == BSTR_OK && blength(vv) == blength(&brand))
                                            {
                                                s->data.ival = rand();
                                            }
                                            else
                                            {
                                                batoi(vv, &s->data.ival);
                                            }
                                        }
#ifdef WITH_HALF_PRECISION
                                        else if (s->type = TEST_STREAM_TYPE_HALF)
                                        {
                                            if (bstrnicmp(vv, &brand, blength(&brand)) == BSTR_OK && blength(vv) == blength(&brand))
                                            {
                                                s->data.f16val = (_Float16)((float)rand() / (float)RAND_MAX);
                                            }
                                            else
                                            {
                                                float tmp;
                                                if (batof(vv, &tmp) == 0)
                                                {
                                                    s->data.f16val = (_Float16)tmp;
                                                }
                                            }
                                        }
#endif
                                        else if (s->type = TEST_STREAM_TYPE_INT64)
                                        {
                                            if (bstrnicmp(vv, &brand, blength(&brand)) == BSTR_OK && blength(vv) == blength(&brand))
                                            {
                                                s->data.i64val = ((int64_t)rand() << 32) | (int64_t)rand();
                                            }
                                            else
                                            {
                                                batoi64(vv, &s->data.i64val);
                                            }
                                        }
                                    }
                                    if (bstrnicmp(vk, &bstrdimsizes, 8) == BSTR_OK)
                                    {
                                        read_yaml_ptt_list(vv, &s->dims);
                                    }
                                    if (bstrnicmp(vk, &bstropts, 7) == BSTR_OK)
                                    {
                                        struct bstrList* tmpl = bstrListCreate();
                                        read_yaml_ptt_list(vv, &tmpl);
                                        // printf("Options: %s\n", bdata(tmpl->entry[0]));
                                        // bstrListPrint(tmpl);
                                        if (bstrnicmp(tmpl->entry[0], &bperthread, blength(&bperthread)) == BSTR_OK && blength(&bperthread) == blength(tmpl->entry[0]))
                                        {
                                            conf->initialization = true;
                                        }
                                        bstrListDestroy(tmpl);
                                    }
                                    //printf("Stream '%s' '%s'\n", bdata(s->name), bdata(vk));
                                    bdestroy(vk);
                                    bdestroy(vv);
                                }
                            }
                            //bdestroy(sk);
                            bdestroy(sv);
                            bstrListDestroy(vars);
                        }
                    }
                    conf->num_streams = streams->qty;
                }
                bstrListDestroy(streams);
            }
            else if (bstrnicmp(k, &bname, blength(&bname)) == BSTR_OK)
            {
                conf->name = bstrcpy(v);
                btrimws(conf->name);
                //printf("Set name '%s'\n", bdata(conf->name));
            }
            else if (bstrnicmp(k, &bdesc, blength(&bdesc)) == BSTR_OK)
            {
                conf->description = bstrcpy(v);
                btrimws(conf->description);
                //printf("Set description '%s'\n", bdata(conf->description));
            }
            else if (bstrnicmp(k, &blang, blength(&blang)) == BSTR_OK)
            {
                conf->language = bstrcpy(v);
                btrimws(conf->language);
            }
            else if (bstrnicmp(k, &bvars, blength(&bvars)) == BSTR_OK)
            {
                conf->num_vars = read_yaml_ptt_dict(v, &conf->vars);
            }
            else if (bstrnicmp(k, &bconstants, blength(&bconstants)) == BSTR_OK)
            {
                conf->num_constants = read_yaml_ptt_dict(v, &conf->constants);
            }
            else if (bstrnicmp(k, &bmetrics, blength(&bmetrics)) == BSTR_OK)
            {
                conf->num_metrics = read_yaml_ptt_dict(v, &conf->metrics);
            }
            else if (bstrnicmp(k, &bparams, blength(&bparamopts)) == BSTR_OK)
            {
                struct bstrList* params = NULL;
                read_obj(v, &params);
                conf->params = malloc(params->qty * sizeof(TestConfigParameter));
                if (conf->params)
                {
                    for (int l = 0; l < params->qty; l++)
                    {
                        TestConfigParameter *p = &conf->params[l];
                        p->name = NULL;
                        p->description = NULL;
                        p->defvalue = NULL;
                        p->options = bstrListCreate();
                        bstring pv;
                        ret = read_keyvalue(params->entry[l], &p->name, &pv);
                        if (ret == 0)
                        {
                            struct bstrList* pvars = NULL;
                            read_obj(pv, &pvars);
                            for (int m = 0; m < pvars->qty; m++)
                            {
                                bstring vk, vv;
                                ret = read_keyvalue(pvars->entry[m], &vk, &vv);
                                
                                if (bstrnicmp(vk, &bparamdesc, blength(&bparamdesc)) == BSTR_OK)
                                {
                                    p->description = bstrcpy(vv);
                                    btrimws(p->description);
                                }
                                else if (bstrnicmp(vk, &bparamopts, blength(&bparamopts)) == BSTR_OK)
                                {
                                    read_yaml_ptt_list(vv, &p->options);
                                }
                                if (bstrnicmp(vk, &bparamdef, blength(&bparamdef)) == BSTR_OK)
                                {
                                    p->defvalue = bstrcpy(vv);
                                    btrimws(p->defvalue);
                                }
                                bdestroy(vk);
                                bdestroy(vv);
                            }
                            bstrListDestroy(pvars);
                        }
                        bdestroy(pv);
                    }
                }
                conf->num_params = params->qty;
                bstrListDestroy(params);
            }
            else if (bstrnicmp(k, &bflags, blength(&bflags)) == BSTR_OK)
            {
                btrimws(v);
                read_yaml_ptt_list(v, &conf->flags);
            }
            else if (bstrnicmp(k, &brequirewg, blength(&brequirewg)) == BSTR_OK)
            {
                btrimws(v);
                if (bstrnicmp(v, &btrue, blength(&btrue)) == BSTR_OK)
                {
                    conf->requirewg = true;
                }
                // printf("requirewg: %d\n", conf->requirewg);
            }
            else if (bstrnicmp(k, &bthreads, 7) == BSTR_OK)
            {
                // printf("Threads\n");
                struct bstrList* thread_items = NULL;
                read_obj(v, &thread_items);
                conf->threads = malloc(sizeof(TestConfigThread));
                if (conf->threads)
                {
                    TestConfigThread* t = conf->threads;
                    t->offsets = bstrListCreate();
                    t->sizes = bstrListCreate();
                    for (int l = 0; l < thread_items->qty; l++)
                    {
                        bstring tk, tv;
                        ret = read_keyvalue(thread_items->entry[l], &tk, &tv);
                        // printf("'%s'\n", bdata(tk));
                        btrimws(tv);
                        if (ret == 0)
                        {
                            if (bstrnicmp(tk, &bthreadsoff, 7) == BSTR_OK)
                            {
                                read_yaml_ptt_list(tv, &t->offsets);
                                // bstrListPrint(t->offsets);
                            }
                            else if (bstrnicmp(tk, &bthreadssize, 5) == BSTR_OK)
                            {
                                read_yaml_ptt_list(tv, &t->sizes);
                                // bstrListPrint(t->sizes);
                            }
                        }

                        bdestroy(tk);
                        bdestroy(tv);
                    }

                    conf->num_threads = 1;
                }

                bstrListDestroy(thread_items);
            }
            bdestroy(k);
            bdestroy(v);
        }
    }

    bstrListDestroy(objs);

    conf->code = read_code(bptt);
    btrimws(conf->code);

    bdestroy(bptt);

    *config = conf;
    return 0;
}

void close_vars(int num_vars, TestConfigVariable* vars)
{
    int i = 0;
    if (num_vars > 0 && vars)
    {
        DEBUG_PRINT(DEBUGLEV_DEVELOP, Destroying %d variables, num_vars);
        for (i = 0; i < num_vars; i++)
        {
            TestConfigVariable* s = &vars[i];
            if (s->name) bdestroy(s->name);
            if (s->value) bdestroy(s->value);
        }
        free(vars);
        vars = NULL;
    }
}

void close_threads(int num_threads, TestConfigThread* threads)
{
    int i = 0;
    if (num_threads > 0 && threads)
    {
        DEBUG_PRINT(DEBUGLEV_DEVELOP, Destroying %d thread items, num_threads);
        for (i = 0; i < num_threads; i++)
        {
            TestConfigThread* t = &threads[i];
            if (t->offsets) bstrListDestroy(t->offsets);
            if (t->sizes) bstrListDestroy(t->sizes);
        }
        free(threads);
    }
}

void close_streams(int num_streams, TestConfigStream* streams)
{
    int i = 0;
    if (num_streams > 0 && streams)
    {
        DEBUG_PRINT(DEBUGLEV_DEVELOP, Destroying %d streams, num_streams);
        for (i = 0; i < num_streams; i++)
        {
            TestConfigStream* s = &streams[i];
            if (s->name) bdestroy(s->name);
            if (s->btype) bdestroy(s->btype);
            if (s->dims) bstrListDestroy(s->dims);
        }
        free(streams);
    }
}

void close_parameters(int num_params, TestConfigParameter* params)
{
    int i = 0;
    if (num_params > 0 && params)
    {
        DEBUG_PRINT(DEBUGLEV_DEVELOP, Destroying %d parameters, num_params);
        for (i = 0; i < num_params; i++)
        {
            TestConfigParameter* p = &params[i];
            if (p->name) bdestroy(p->name);
            if (p->description) bdestroy(p->description);
            if (p->options) bstrListDestroy(p->options);
            if (p->defvalue) bdestroy(p->defvalue);
        }
        free(params);
    }
}

void close_yaml_ptt(TestConfig_t config)
{
    if (config)
    {
        DEBUG_PRINT(DEBUGLEV_DEVELOP, Destroying name in TestConfig);
        bdestroy(config->name);
        DEBUG_PRINT(DEBUGLEV_DEVELOP, Destroying description in TestConfig);
        bdestroy(config->description);
        DEBUG_PRINT(DEBUGLEV_DEVELOP, Destroying language in TestConfig);
        bdestroy(config->language);
        if (config->code)
        {
            DEBUG_PRINT(DEBUGLEV_DEVELOP, Destroying code in TestConfig);
            bdestroy(config->code);
            config->code = NULL;
        }
        DEBUG_PRINT(DEBUGLEV_DEVELOP, Destroying constants in TestConfig);
        close_vars(config->num_constants, config->constants);
        DEBUG_PRINT(DEBUGLEV_DEVELOP, Destroying variables in TestConfig);
        close_vars(config->num_vars, config->vars);
        DEBUG_PRINT(DEBUGLEV_DEVELOP, Destroying metrics in TestConfig);
        close_vars(config->num_metrics, config->metrics);
        DEBUG_PRINT(DEBUGLEV_DEVELOP, Destroying streams in TestConfig);
        close_streams(config->num_streams, config->streams);
        DEBUG_PRINT(DEBUGLEV_DEVELOP, Destroying thread items in TestConfig);
        close_threads(config->num_threads, config->threads);
        DEBUG_PRINT(DEBUGLEV_DEVELOP, Destroying parameters in TestConfig);
        close_parameters(config->num_params, config->params);
        DEBUG_PRINT(DEBUGLEV_DEVELOP, Destroying flags in TestConfig);
        bstrListDestroy(config->flags);
        DEBUG_PRINT(DEBUGLEV_DEVELOP, Destroying TestConfig);
        free(config);
        config = NULL;
    }
}
