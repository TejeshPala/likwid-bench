#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>

#include "error.h"
#include "bstrlib.h"
#include "bstrlib_helper.h"
#include "dynload.h"
#include "test_types.h"


bstring get_compiler(bstring candidates)
{
    bstring compiler = NULL;
    bstring path = bfromcstr(getenv("PATH"));
    struct bstrList *plist = NULL;
    struct bstrList *clist = NULL;
    int (*ownaccess)(const char*, int) = access;

    plist = bsplit(path, ':');
    clist = bsplit(candidates, ',');
    for (int i = 0; i < NUM_COMPILER_CANDIDATES; i++)
    {
        int found = 0;
        for (int j = 0; j < clist->qty; j++)
        {
            if (bstrcmp(&compiler_candidates[i], clist->entry[j]) == BSTR_OK)
            {
                found = 1;
                break;
            }
        }
        if (!found)
        {
            bstrListAdd(clist, &compiler_candidates[i]);
        }
    }

    for (int i = 0; i < plist->qty && (!compiler); i++)
    {
        for (int j = 0; j < clist->qty && (!compiler); j++)
        {
            bstring tmp = bformat("%s/%s", bdata(plist->entry[i]), bdata(clist->entry[j]));
            if (!ownaccess(bdata(tmp), R_OK|X_OK))
            {
                compiler = bstrcpy(tmp);
            }
            bdestroy(tmp);
        }
    }

    bdestroy(path);
    bstrListDestroy(plist);
    bstrListDestroy(clist);
    return compiler;
}

int open_function(RuntimeThreadConfig* thread)
{
    char *error = NULL;
    void* (*owndlopen)(const char*, int) = dlopen;
    void* (*owndlsym)(void*, const char*) = dlsym;
    int (*ownaccess)(const char*, int) = access;
    if (!ownaccess(bdata(thread->testconfig->objfile), F_OK))
    {
        dlerror();
        DEBUG_PRINT(DEBUGLEV_DEVELOP, Opening object file %s, bdata(thread->testconfig->objfile));
        thread->testconfig->dlhandle = owndlopen(bdata(thread->testconfig->objfile), RTLD_LAZY);
        if (!thread->testconfig->dlhandle) {
            ERROR_PRINT(Error dynloading file %s: %s, bdata(thread->testconfig->objfile), dlerror());
            return -1;
        }
        dlerror();
        DEBUG_PRINT(DEBUGLEV_DEVELOP, Loading function %s from object file %s, bdata(thread->testconfig->functionname), bdata(thread->testconfig->objfile));
        thread->testconfig->function = owndlsym(thread->testconfig->dlhandle, bdata(thread->testconfig->functionname));
        if ((error = dlerror()) != NULL)  {
            dlclose(thread->testconfig->dlhandle);
            thread->testconfig->dlhandle = NULL;
            ERROR_PRINT(Error dynloading function %s from file %s: %s, bdata(thread->testconfig->functionname), bdata(thread->testconfig->objfile), error);
            return -1;
        }
        DEBUG_PRINT(DEBUGLEV_DEVELOP, Function pointer %p, thread->testconfig->function);
        dlerror();
    }
    else
    {
        return -ENOENT;
    }
    return 0;
}

int close_function(RuntimeThreadConfig* thread)
{
    if (thread->testconfig->dlhandle)
    {
        dlclose(thread->testconfig->dlhandle);
        thread->testconfig->dlhandle = NULL;
        thread->testconfig->function = NULL;
    }
    if (thread->testconfig->objfile) bdestroy(thread->testconfig->objfile);
    if (thread->testconfig->functionname) bdestroy(thread->testconfig->functionname);
    if (thread->testconfig->flags) bdestroy(thread->testconfig->flags);
    if (thread->testconfig->compiler) bdestroy(thread->testconfig->compiler);
    free(thread->testconfig);
    thread->testconfig = NULL;
    return 0;
}

int dump_assembly(RuntimeThreadConfig* thread, bstring outfile)
{
    return write_bstrList_to_file(thread->codelines, bdata(outfile));
}

void fill_keylist(mpointer key, mpointer value, mpointer user_data)
{
    bstring bkey = (bstring)key;
    struct bstrList* list = (struct bstrList*)user_data;
    bstrListAdd(list, bkey);
}

int sortfunc(const struct tagbstring * left, const struct tagbstring * right)
{
    if (blength(left) < blength(right)) return -1;
    else if (blength(left) > blength(right)) return 1;
    else return bstrcmp(left, right);
    return 0;
}

int bmkstemp(bstring template)
{
    int (*mymkstemp)(char* fname) = mkstemp;
    if (template && bdata(template) != NULL)
    {
        return mymkstemp(bdata(template));
    }
    errno = EINVAL;
    return -1;
}

int bunlink(bstring filename)
{
    int (*myunlink)(const char* fname) = unlink;
    if (filename && bdata(filename) != NULL)
    {
        return myunlink(bdata(filename));
    }
    errno = EINVAL;
    return -1;
}

int dynload_create_runtime_test_config(RuntimeConfig* rcfg, RuntimeWorkgroupConfig* wcfg)
{
    int ret = 0;
    bstring flags = bfromcstr("-fPIC -shared");
    bstring compiler = get_compiler(rcfg->compiler);
    for (int t = 0; t < wcfg->num_threads; t++)
    {
        RuntimeThreadConfig* thread = &wcfg->threads[t];
        int fd = 0;
        bstring filetemplate = NULL;
        bstring asmfile = NULL;
        bstring objfile = NULL;
        struct bstrList* wcodelines = NULL;
        if (!compiler)
        {
            return -ENOENT;
        }

        filetemplate = bformat("/tmp/likwid-bench-t%d-XXXXXX", t);
        fd = bmkstemp(filetemplate);
        if (fd == -1)
        {
            ERROR_PRINT(Failed to get temporary file name for assembly);
            bdestroy(compiler);
            bdestroy(filetemplate);
            return -1;
        }
        DEBUG_PRINT(DEBUGLEV_DEVELOP, Generated file name is '%s', bdata(filetemplate));

        asmfile = bstrcpy(filetemplate);
        close(fd);
        if (bunlink(filetemplate) == -1)
        {
            ERROR_PRINT(Failed to unlink '%s', bdata(filetemplate));
            bdestroy(compiler);
            bdestroy(filetemplate);
            return -1;
        }
        bdestroy(filetemplate);
        bconchar(asmfile, '.');
        objfile = bstrcpy(asmfile);
        bconchar(asmfile, 's');
        bconchar(objfile, 'o');

        wcodelines = bstrListCopy(thread->codelines);
        struct bstrList *valkeys = bstrListCreate();
        struct bstrList *varkeys = bstrListCreate();
        struct bstrList *sorted_valkeys = NULL;
        struct bstrList *sorted_varkeys = NULL;
        foreach_in_bmap(wcfg->results[t].values, fill_keylist, valkeys);
        foreach_in_bmap(wcfg->results[t].variables, fill_keylist, varkeys);

        bstrListSortFunc(valkeys, sortfunc, &sorted_valkeys);
        bstrListSortFunc(varkeys, sortfunc, &sorted_varkeys);
        bstrListDestroy(valkeys);
        bstrListDestroy(varkeys);

        for (int i = 0; i < wcodelines->qty; i++)
        {
            if (bchar(wcodelines->entry[i], 0) == '#') continue;
            for (int j = sorted_valkeys->qty - 1; j >= 0; j--)
            {
                if (binstr(wcodelines->entry[i], 0, sorted_valkeys->entry[j]) != BSTR_ERR && bchar(wcodelines->entry[i], 0) != '.')
                {
                    bstring val = NULL;
                    ret = get_bmap_by_key(wcfg->results[t].values, sorted_valkeys->entry[j], (void**)&val);
                    if (ret == 0)
                    {
                        DEBUG_PRINT(DEBUGLEV_DEVELOP, Replacing '%s' with '%s' in '%s', bdata(sorted_valkeys->entry[j]), bdata(val), bdata(wcodelines->entry[i]));
                        bfindreplace(wcodelines->entry[i], sorted_valkeys->entry[j], val, 0);
                    }
                }
            }
            for (int j = sorted_varkeys->qty - 1; j >= 0; j--)
            {
                if (binstr(wcodelines->entry[i], 0, sorted_varkeys->entry[j]) != BSTR_ERR && bchar(wcodelines->entry[i], 0) != '.')
                {
                    bstring val = NULL;
                    ret = get_bmap_by_key(wcfg->results[t].variables, sorted_varkeys->entry[j], (void**)&val);
                    if (ret == 0)
                    {
                        DEBUG_PRINT(DEBUGLEV_DEVELOP, Replacing '%s' with '%s' in '%s', bdata(sorted_varkeys->entry[j]), bdata(val), bdata(wcodelines->entry[i]));
                        bfindreplace(wcodelines->entry[i], sorted_varkeys->entry[j], val, 0);
                    }
                }
            }
        }

        bstrListDestroy(sorted_valkeys);
        bstrListDestroy(sorted_varkeys);

        ret = write_bstrList_to_file(wcodelines, bdata(asmfile));
        if (ret < 0)
        {
            ERROR_PRINT(Failed to write assembly to file %s, bdata(asmfile));
            bdestroy(asmfile);
            bdestroy(objfile);
            bstrListDestroy(wcodelines);
            return -1;
        }

        bstring cmd = bformat("%s %s %s -o %s 2>&1", bdata(compiler), bdata(flags), bdata(asmfile), bdata(objfile));
        DEBUG_PRINT(DEBUGLEV_DEVELOP, CMD %s, bdata(cmd));
        FILE * fp = popen(bdata(cmd), "r");
        if (fp)
        {
            bstring bstdout = bread ((bNread) fread, fp);
            if (blength(bstdout) > 0)
            {
                btrimws(bstdout);
                struct bstrList* errlist = bsplit(bstdout, '\n');
                for (int i = 0; i < errlist->qty; i++)
                {
                    ERROR_PRINT(%s, bdata(errlist->entry[i]));
                }
                bstrListDestroy(errlist);
            }
            bdestroy(bstdout);
            ret = pclose(fp);
        }
        else
        {
            ret = errno;
            ERROR_PRINT(Failed to execute: %s, bdata(cmd))
        }
        bdestroy(cmd);
        bdestroy(asmfile);
        if (global_verbosity == DEBUGLEV_DEVELOP)
        {
            for (int i = 0; i < wcodelines->qty; i++)
            {
                btrimws(wcodelines->entry[i]);
                if (blength(wcodelines->entry[i]) > 0)
                {
                    DEBUG_PRINT(DEBUGLEV_DEVELOP, Code(hwthread %d): %s, t, bdata(wcodelines->entry[i]));
                }
            }
        }
        bstrListDestroy(wcodelines);

        thread->testconfig->objfile = bstrcpy(objfile);
        thread->testconfig->compiler = bstrcpy(compiler);
        thread->testconfig->flags = bstrcpy(flags);
        thread->testconfig->functionname = bstrcpy(rcfg->testname);
        thread->testconfig->dlhandle = NULL;
        thread->testconfig->function = NULL;
        bdestroy(objfile);
    }
    bdestroy(flags);
    bdestroy(compiler);
    return ret;
}
