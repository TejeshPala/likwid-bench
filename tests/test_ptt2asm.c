#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "error.h"
#include "bstrlib.h"
#include "bstrlib_helper.h"
#include "ptt2asm.h"
#include "test_types.h"

int global_verbosity = DEBUGLEV_DEVELOP;


int main(int argc, char* argv[])
{
    struct bstrList* out = bstrListCreate();
    TestConfig tcfg;
    memset(&tcfg, 0, sizeof(TestConfig));
    tcfg.name = bfromcstr("doubleload");
    tcfg.code = bfromcstr("xor xmm0, xmm0\nLOOP(loop, rax=0, <, rdi=N, UNROLL_FACTOR)\nxor xmm0, xmm0\nDUMMY(myunroll)\nmovsd    xmm0, [STR0 + rax * 8]\nmovsd    xmm1, [STR0 + rax * 8 + 8]\nmovsd    xmm2, [STR0 + rax * 8 + 16]\nmovsd    xmm3, [STR0 + rax * 8 + 24]\nmovsd    xmm4, [STR0 + rax * 8 + 32]\nmovsd    xmm5, [STR0 + rax * 8 + 40]\nmovsd    xmm6, [STR0 + rax * 8 + 48]\nmovsd    xmm7, [STR0 + rax * 8 + 56]\nDUMMYEND(myunroll)\nxor xmm0, xmm0\nLOOPEND(loop)\nxor xmm0, xmm0\nxor xmm0, xmm0\nLOOP(loop2, rax=0, <, rdi=N, UNROLL_FACTOR)\nxor xmm0, xmm0\nDUMMY(myunroll2)\nmovsd    xmm0, [STR0 + rax * 8]\nmovsd    xmm1, [STR0 + rax * 8 + 8]\nmovsd    xmm2, [STR0 + rax * 8 + 16]\nmovsd    xmm3, [STR0 + rax * 8 + 24]\nmovsd    xmm4, [STR0 + rax * 8 + 32]\nmovsd    xmm5, [STR0 + rax * 8 + 40]\nmovsd    xmm6, [STR0 + rax * 8 + 48]\nmovsd    xmm7, [STR0 + rax * 8 + 56]\nDUMMYEND(myunroll2)\nxor xmm0, xmm0\nLOOPEND(loop2)\nxor xmm0, xmm0\n");
    //tcfg.code = bfromcstr("xor xmm0, xmm0\nLOOP(loop, rax=0, <, rdi=N, UNROLL_FACTOR)\nxor xmm0, xmm0\nDUMMY(myunroll)\nmovsd    xmm0, [STR0 + rax * 8]\nmovsd    xmm1, [STR0 + rax * 8 + 8]\nmovsd    xmm2, [STR0 + rax * 8 + 16]\nmovsd    xmm3, [STR0 + rax * 8 + 24]\nmovsd    xmm4, [STR0 + rax * 8 + 32]\nmovsd    xmm5, [STR0 + rax * 8 + 40]\nmovsd    xmm6, [STR0 + rax * 8 + 48]\nmovsd    xmm7, [STR0 + rax * 8 + 56]\nDUMMYEND(myunroll)\nxor xmm0, xmm0\nLOOPEND(loop)\n");
    prepare_ptt(&tcfg, out);
    printf("Body:\n");
    for (int i = 0; i < out->qty; i++)
    {
        printf("%s\n", bdata(out->entry[i]));
    }
    bstrListDestroy(out);
    out = bstrListCreate();
    generate_code(&tcfg, out);
    printf("Final:\n");
    for (int i = 0; i < out->qty; i++)
    {
        printf("%s\n", bdata(out->entry[i]));
    }
    bdestroy(tcfg.code);
    bdestroy(tcfg.name);
    bstrListDestroy(out);
    return 0;
}
