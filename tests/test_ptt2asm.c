#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

#include "error.h"
#include "bstrlib.h"
#include "bstrlib_helper.h"
#include "ptt2asm.h"
#include "test_types.h"

int global_verbosity = DEBUGLEV_DEVELOP;

static struct tagbstring testfolder = bsStatic("./ptt2asm");

static bstring get_reference_file(bstring pttfile)
{
    bstring o = bstrcpy(pttfile);
    btrunc(o,blength(o)-3);
    bconchar(o,'s');
    return o;
}

static int get_tests(bstring folder, struct bstrList** testlist)
{
    DIR* dp = NULL;
    struct dirent *ep = NULL;
    if ((!folder) || (blength(folder) == 0)) return -EINVAL;
    const char* cfolder = bdata(folder);
    if (!cfolder) return -EINVAL;

    struct bstrList* out = bstrListCreate();

    dp = opendir(cfolder);
    if (dp != NULL)
    {
        while ((ep = readdir(dp)))
        {
            if (ep->d_type != DT_REG) continue;
            int slen = strlen(ep->d_name);
            if (slen < 4) continue;
            if (strncmp(&ep->d_name[slen-4], ".ptt", 4) == 0)
            {
                bstrListAddChar(out, ep->d_name);
            }
        }
        closedir(dp);
    }
    *testlist = out;
    return 0;
}


int main(int argc, char* argv[])
{
/*    struct bstrList* out = bstrListCreate();*/
/*    TestConfig tcfg;*/
/*    memset(&tcfg, 0, sizeof(TestConfig));*/
/*    tcfg.name = bfromcstr("doubleload");*/
/*    tcfg.code = bfromcstr("xor xmm0, xmm0\nLOOP(loop, rax=0, <, rdi=N, UNROLL_FACTOR)\nxor xmm0, xmm0\nDUMMY(myunroll)\nmovsd    xmm0, [STR0 + rax * 8]\nmovsd    xmm1, [STR0 + rax * 8 + 8]\nmovsd    xmm2, [STR0 + rax * 8 + 16]\nmovsd    xmm3, [STR0 + rax * 8 + 24]\nmovsd    xmm4, [STR0 + rax * 8 + 32]\nmovsd    xmm5, [STR0 + rax * 8 + 40]\nmovsd    xmm6, [STR0 + rax * 8 + 48]\nmovsd    xmm7, [STR0 + rax * 8 + 56]\nDUMMYEND(myunroll)\nxor xmm0, xmm0\nLOOPEND(loop)\nxor xmm0, xmm0\nxor xmm0, xmm0\nLOOP(loop2, rax=0, <, rdi=N, UNROLL_FACTOR)\nxor xmm0, xmm0\nDUMMY(myunroll2)\nmovsd    xmm0, [STR0 + rax * 8]\nmovsd    xmm1, [STR0 + rax * 8 + 8]\nmovsd    xmm2, [STR0 + rax * 8 + 16]\nmovsd    xmm3, [STR0 + rax * 8 + 24]\nmovsd    xmm4, [STR0 + rax * 8 + 32]\nmovsd    xmm5, [STR0 + rax * 8 + 40]\nmovsd    xmm6, [STR0 + rax * 8 + 48]\nmovsd    xmm7, [STR0 + rax * 8 + 56]\nDUMMYEND(myunroll2)\nxor xmm0, xmm0\nLOOPEND(loop2)\nxor xmm0, xmm0\n");*/
/*    //tcfg.code = bfromcstr("xor xmm0, xmm0\nLOOP(loop, rax=0, <, rdi=N, UNROLL_FACTOR)\nxor xmm0, xmm0\nDUMMY(myunroll)\nmovsd    xmm0, [STR0 + rax * 8]\nmovsd    xmm1, [STR0 + rax * 8 + 8]\nmovsd    xmm2, [STR0 + rax * 8 + 16]\nmovsd    xmm3, [STR0 + rax * 8 + 24]\nmovsd    xmm4, [STR0 + rax * 8 + 32]\nmovsd    xmm5, [STR0 + rax * 8 + 40]\nmovsd    xmm6, [STR0 + rax * 8 + 48]\nmovsd    xmm7, [STR0 + rax * 8 + 56]\nDUMMYEND(myunroll)\nxor xmm0, xmm0\nLOOPEND(loop)\n");*/
/*    prepare_ptt(&tcfg, out);*/
/*    printf("Body:\n");*/
/*    for (int i = 0; i < out->qty; i++)*/
/*    {*/
/*        printf("%s\n", bdata(out->entry[i]));*/
/*    }*/
/*    bstrListDestroy(out);*/
/*    out = bstrListCreate();*/
/*    generate_code(&tcfg, out);*/
/*    printf("Final:\n");*/
/*    for (int i = 0; i < out->qty; i++)*/
/*    {*/
/*        printf("%s\n", bdata(out->entry[i]));*/
/*    }*/
/*    bdestroy(tcfg.code);*/
/*    bdestroy(tcfg.name);*/
/*    bstrListDestroy(out);*/

    struct tagbstring name = bsStatic("testkernel");
    struct bstrList* out = NULL;

    int allerrors = 0;
    int alltests = 0;
    get_tests(&testfolder, &out);
    if (out)
    {
        alltests = out->qty;
        for (int i = 0; i< out->qty; i++)
        {
            int err = 0;
            TestConfig tcfg;
            bstring ref = get_reference_file(out->entry[i]);
            if (ref)
            {
                bstring testabspath = bformat("%s/%s", bdata(&testfolder), bdata(out->entry[i]));
                bstring refabspath = bformat("%s/%s", bdata(&testfolder), bdata(ref));
                if (testabspath && refabspath)
                {
                    tcfg.name = &name;
                    tcfg.code = read_file(bdata(testabspath));
                    if (tcfg.code)
                    {
                        btrimws(tcfg.code);
                        struct bstrList* codelines = bstrListCreate();
                        prepare_ptt(&tcfg, codelines);
                        bstring refcode = read_file(bdata(refabspath));
                        if (refcode)
                        {
                            btrimws(refcode);
                            struct bstrList* reflines = NULL;
                            if (blength(refcode) > 0)
                            {
                                reflines = bsplit(refcode, '\n');
                            }
                            else
                            {
                                reflines = bstrListCreate();
                            }
                            if (codelines->qty != reflines->qty)
                            {
                                fprintf(stderr, "%s: Generated %d lines of code but the reference code has %d lines\n", bdata(out->entry[i]), codelines->qty, reflines->qty);
                                err = 1;
                            }
                            if (reflines->qty > 0 && codelines->qty > 0)
                            {
                                for (int j = 0; j < reflines->qty; j++)
                                {
                                    if (bstrcmp(codelines->entry[j], reflines->entry[j]) != BSTR_OK)
                                    {
                                        err = 1;
                                        break;
                                    }
                                }
                            }
                            bstrListDestroy(reflines);
                            bdestroy(refcode);
                        }
                        bstrListDestroy(codelines);
                        bdestroy(tcfg.code);
                        tcfg.code = NULL;
                    }
                    bdestroy(testabspath);
                    bdestroy(refabspath);
                }
                bdestroy(ref);
            }
            allerrors += err;
        }
        bstrListDestroy(out);
    }
    printf("%d Tests out of %d failed\n", allerrors, alltests);
    return 0;
}
