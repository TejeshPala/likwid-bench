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


static bstring get_compiler(bstring candidates)
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


static int compile_file(bstring compiler, bstring flags, bstring asmfile, bstring objfile)
{
    int exitcode = 0;
    if (blength(compiler) == 0 || blength(asmfile) == 0)
        return -1;

    bstring cmd = bformat("%s %s %s -o %s", bdata(compiler), bdata(flags), bdata(asmfile), bdata(objfile));
    DEBUG_PRINT(DEBUGLEV_DEVELOP, CMD %s, bdata(cmd));
    FILE * fp = popen(bdata(cmd), "r");
    if (fp)
    {
        bstring bstdout = bread ((bNread) fread, fp);
        if (blength(bstdout) > 0)
        {
            ERROR_PRINT(%s, bdata(bstdout))
        }
        bdestroy(bstdout);
        exitcode = pclose(fp);
    }
    else
    {
        exitcode = errno;
        ERROR_PRINT(Failed to execute: %s, bdata(cmd))
    }
    bdestroy(cmd);

    return exitcode;
}


static int open_function(RuntimeConfig *runcfg)
{
    char *error = NULL;
    void* (*owndlopen)(const char*, int) = dlopen;
    void* (*owndlsym)(void*, const char*) = dlsym;

    dlerror();
    runcfg->testconfig.dlhandle = owndlopen(bdata(runcfg->testconfig.objfile), RTLD_LAZY);
    if (!runcfg->testconfig.dlhandle) {
        ERROR_PRINT(Error dynloading file %s: %s, bdata(runcfg->testconfig.objfile), dlerror());
        return -1;
    }
    dlerror();
    runcfg->testconfig.function = owndlsym(runcfg->testconfig.dlhandle, bdata(runcfg->testname));
    if ((error = dlerror()) != NULL)  {
        dlclose(runcfg->testconfig.dlhandle);
        ERROR_PRINT(Error dynloading function %s: %s, bdata(runcfg->testconfig.objfile), error);
        return -1;
    }
    dlerror();

    return 0;
}

static int close_function(RuntimeConfig *runcfg)
{
    if (runcfg->testconfig.dlhandle)
    {
        dlclose(runcfg->testconfig.dlhandle);
        runcfg->testconfig.dlhandle = NULL;
        runcfg->testconfig.function;
    }
    return 0;
}

static int dump_assembly(RuntimeConfig *runcfg, bstring outfile)
{
    int ret = 0;
    char buf[1024];

    if (access(runcfg->testconfig.asmfile, R_OK))
    {
        ERROR_PRINT(Assembly file %s does not exist, bdata(runcfg->testconfig.asmfile));
        return -errno;
    }
    FILE* infp = fopen(bdata(runcfg->testconfig.asmfile), "r");
    if (infp)
    {
        FILE* outfp = fopen(bdata(outfile), "w");
        if (outfp)
        {
            for (;;)
            {
                ret = fread(buf, sizeof(char), sizeof(buf), infp);
                if (ret <= 0)
                {
                    break;
                }
                ret = fwrite(buf, sizeof(char), ret, outfp);
            }
            fclose(outfp);
        }
        fclose(infp);
    }
    return 0;
}
