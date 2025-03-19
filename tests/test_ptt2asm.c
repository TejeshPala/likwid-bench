#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "allocator.h"
#include "error.h"
#include "bstrlib.h"
#include "bstrlib_helper.h"
#include "ptt2asm.h"
#include "test_types.h"

int global_verbosity = DEBUGLEV_DEVELOP;

int main(int argc, char* argv[])
{
    struct bstrList* out = bstrListCreate();
    struct bstrList* regs = bstrListCreate();
    RuntimeConfig runcfg;
    memset(&runcfg, 0, sizeof(RuntimeConfig));
    TestConfig tcfg;
    memset(&tcfg, 0, sizeof(TestConfig));
    static struct tagbstring default_stream_name = bsStatic("STR0");
    int ival = 10;
    tcfg.num_streams = 1;
    tcfg.streams = (TestConfigStream*)malloc(sizeof(TestConfigStream) * tcfg.num_streams);
    tcfg.streams[0] = (TestConfigStream){
        .name = &default_stream_name,
        .num_dims = 1,
        .type = TEST_STREAM_TYPE_INT,
        .btype = NULL,
        .data = {ival},
        };
    tcfg.name = bfromcstr("doubleload");
    tcfg.code = bfromcstr("xor xmm0, xmm0\nLOOP(loop, rax=0, <, rdi=N, UNROLL_FACTOR)\nxor xmm0, xmm0\nDUMMY(myunroll)\nmovsd    xmm0, [STR0 + rax * 8]\nmovsd    xmm1, [STR0 + rax * 8 + 8]\nmovsd    xmm2, [STR0 + rax * 8 + 16]\nmovsd    xmm3, [STR0 + rax * 8 + 24]\nmovsd    xmm4, [STR0 + rax * 8 + 32]\nmovsd    xmm5, [STR0 + rax * 8 + 40]\nmovsd    xmm6, [STR0 + rax * 8 + 48]\nmovsd    xmm7, [STR0 + rax * 8 + 56]\nDUMMYEND(myunroll)\nxor xmm0, xmm0\nLOOPEND(loop)\nxor xmm0, xmm0\nxor xmm0, xmm0\nLOOP(loop2, rax=0, <, rdi=N, UNROLL_FACTOR)\nxor xmm0, xmm0\nDUMMY(myunroll2)\nmovsd    xmm0, [STR0 + rax * 8]\nmovsd    xmm1, [STR0 + rax * 8 + 8]\nmovsd    xmm2, [STR0 + rax * 8 + 16]\nmovsd    xmm3, [STR0 + rax * 8 + 24]\nmovsd    xmm4, [STR0 + rax * 8 + 32]\nmovsd    xmm5, [STR0 + rax * 8 + 40]\nmovsd    xmm6, [STR0 + rax * 8 + 48]\nmovsd    xmm7, [STR0 + rax * 8 + 56]\nDUMMYEND(myunroll2)\nxor xmm0, xmm0\nLOOPEND(loop2)\nxor xmm0, xmm0\n");
    //tcfg.code = bfromcstr("xor xmm0, xmm0\nLOOP(loop, rax=0, <, rdi=N, UNROLL_FACTOR)\nxor xmm0, xmm0\nDUMMY(myunroll)\nmovsd    xmm0, [STR0 + rax * 8]\nmovsd    xmm1, [STR0 + rax * 8 + 8]\nmovsd    xmm2, [STR0 + rax * 8 + 16]\nmovsd    xmm3, [STR0 + rax * 8 + 24]\nmovsd    xmm4, [STR0 + rax * 8 + 32]\nmovsd    xmm5, [STR0 + rax * 8 + 40]\nmovsd    xmm6, [STR0 + rax * 8 + 48]\nmovsd    xmm7, [STR0 + rax * 8 + 56]\nDUMMYEND(myunroll)\nxor xmm0, xmm0\nLOOPEND(loop)\n");
    runcfg.tcfg = &tcfg;
    prepare_ptt(&tcfg, out, regs);
    printf("Body:\n");
    for (int i = 0; i < out->qty; i++)
    {
        printf("%s\n", bdata(out->entry[i]));
    }
    bstrListDestroy(out);
    out = bstrListCreate();

    // dummy
    RuntimeThreadConfig thread;
    memset(&thread, 0, sizeof(RuntimeThreadConfig));
    thread.num_streams = tcfg.num_streams;
    thread.sdata = (RuntimeStreamConfig*)malloc(sizeof(RuntimeStreamConfig) * tcfg.num_streams);
    for (int i = 0; i < thread.num_streams; i++)
    {
        thread.sdata[i].name = tcfg.streams[i].name;
        thread.sdata[i].type = tcfg.streams[i].type;
    }
    thread.tstreams = (RuntimeThreadStreamConfig*)malloc(sizeof(RuntimeThreadStreamConfig) * tcfg.num_streams);
    for (int i = 0; i < thread.num_streams; i++)
    {
        thread.tstreams[i].tsizes      = 100;
        thread.tstreams[i].toffsets    = 0;
        thread.tstreams[i].tstream_ptr    = 0x00000000;
    }
    

    generate_code(&runcfg, &thread, out);
    printf("Final:\n");
    for (int i = 0; i < out->qty; i++)
    {
        printf("%s\n", bdata(out->entry[i]));
    }
    bdestroy(tcfg.code);
    bdestroy(tcfg.name);
    bstrListDestroy(regs);
    bstrListDestroy(out);
    free(tcfg.streams);
    free(thread.sdata);
    free(thread.tstreams);
    return 0;
}
