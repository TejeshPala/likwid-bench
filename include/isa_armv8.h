/*
 * =======================================================================================
 *      Filename:  isa_armv8.h
 *
 *      Description:  Definitions used for dynamically compile benchmarks for ARMv8 systems
 *
 *      Version:   <VERSION>
 *      Released:  <DATE>
 *
 *      Author:   Thomas Gruber (tg), thomas.roehl@gmail.com
 *      Project:  likwid-bench
 *
 *      Copyright (C) 2019 RRZE, University Erlangen-Nuremberg
 *
 *      This program is free software: you can redistribute it and/or modify it under
 *      the terms of the GNU General Public License as published by the Free Software
 *      Foundation, either version 3 of the License, or (at your option) any later
 *      version.
 *
 *      This program is distributed in the hope that it will be useful, but WITHOUT ANY
 *      WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 *      PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License along with
 *      this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * =======================================================================================
 */
#ifndef LIKWID_BENCH_ISA_ARMV8_H
#define LIKWID_BENCH_ISA_ARMV8_H

#include "bstrlib.h"
#include "bstrlib_helper.h"
#include "test_strings.h"

#define ARCHNAME "armv8"
#define WORDLENGTH 4

static int is_sve_bstr(bstring bstr)
{
    int sve = 0;
    struct tagbstring bsve = bsStatic("sve");
    if (binstrcaseless(bstr, 0, &bsve) != BSTR_ERR)
    {
        sve = 1;
    }
    return sve;
}

static int is_sve(char* string)
{
    int sve = 0;
    bstring bstr = bfromcstr(string);
    sve = is_sve_bstr(bstr);
    bdestroy(bstr);
    return sve;
}



int header(struct bstrList* code, bstring funcname)
{
    int sve = is_sve_bstr(funcname);
    bstring glline = bformat(".global %s", bdata(funcname));
    bstring typeline = bformat(".type %s, @function", bdata(funcname));
    bstring label = bformat("%s :", bdata(funcname));

    if (sve)
    {
        bstrListAddChar(code, ".arch    armv8.2-a+crc+sve");
    }
    else
    {
        bstrListAddChar(code, ".arch    armv8.2-a+crc");
    }

    bstrListAddChar(code, ".data");

    bstrListAddChar(code, ".text");
    bstrListAdd(code, glline);
    bstrListAdd(code, typeline);
    bstrListAdd(code, label);

    bstrListAddChar(code, "stp x29, x30, [sp, -192]!");
    bstrListAddChar(code, "mov x29, sp");
    bstrListAddChar(code, "stp x19, x20, [sp, 16]");
    bstrListAddChar(code, "stp x21, x22, [sp, 32]");
    bstrListAddChar(code, "stp x24, x25, [sp, 48]");
    bstrListAddChar(code, "stp x26, x27, [sp, 64]");
    bstrListAddChar(code, "str x28, [sp, 80]");
    bstrListAddChar(code, "stp d8, d9, [sp, 96]");
    bstrListAddChar(code, "stp d10, d11, [sp, 112]");
    bstrListAddChar(code, "stp d12, d13, [sp, 128]");
    bstrListAddChar(code, "stp d14, d15, [sp, 144]");

    bstring streamline = bformat("%s", bdata(&bstrptr));
    bstrListAdd(code, streamline);
    bdestroy(streamline);

    bstrListAddChar(code, "\n");

    bdestroy(glline);
    bdestroy(typeline);
    bdestroy(label);

    return 0;
}

int footer(struct bstrList* code, bstring funcname)
{
    
    bstrListAddChar(code, ".exit:");

    bstrListAddChar(code, "ldp d14, d15, [sp, 144]");
    bstrListAddChar(code, "ldp d12, d13, [sp, 128]");
    bstrListAddChar(code, "ldp d10, d11, [sp, 112]");
    bstrListAddChar(code, "ldp d8, d9, [sp, 96]");
    bstrListAddChar(code, "ldr x28, [sp, 80]");
    bstrListAddChar(code, "ldp x26, x27, [sp, 64]");
    bstrListAddChar(code, "ldp x24, x25, [sp, 48]");
    bstrListAddChar(code, "ldp x21, x22, [sp, 32]");
    bstrListAddChar(code, "ldp x19, x20, [sp, 16]");
    bstrListAddChar(code, "ldp x29, x30, [sp], 192");

    bstrListAddChar(code, "ret");

    bstring line = bformat(".size %s, .-%s", bdata(funcname), bdata(funcname));
    bstrListAdd(code, line);
    bdestroy(line);

    bstrListAddChar(code, "\n");

    bstrListAddChar(code, "#if defined(__linux__) && defined(__ELF__)");
    bstrListAddChar(code, ".section .note.GNU-stack,\"\",%progbits");
    bstrListAddChar(code, "#endif");

    
    return 0;
}

int loopheader(struct bstrList* code, bstring loopname, bstring loopreg, bstring start, bstring condreg, bstring cond, bstring step)
//int loopheader(struct bstrList* code, char* loopname, int step)
{
    struct tagbstring bzero = bsStatic("0");

    //bstrListAddChar(code, "mov   GPR6, 0");
    bstring initline = bformat("mov %s, %s", bdata(loopreg), bdata(start));
    bstrListAdd(code, initline);
    bdestroy(initline);

    bstring condline = bformat("ldr %s, =#%s", bdata(condreg), bdata(cond));
    bstrListAdd(code, condline);
    bdestroy(condline);

    if (is_sve_bstr(loopname))
    {
        bstring ptrue = bformat("ptrue p0.d"); // activate all vector lanes
        // bstring ptrue = bformat("ptrue p0.d, vl%s", bdata(step)); // Only the vector lanes as per step
        bstrListAdd(code, ptrue);
        bdestroy(ptrue);

        // bstrListAddChar(code, "whilelo  p0.d, GPR6, ARG1");
        bstring whilelo = bformat("whilelo p0.d, %s, %s", bdata(loopreg), bdata(condreg));
        bstrListAdd(code, whilelo);
        bdestroy(whilelo);

        // the predicates copy would be essential if unroll > 1, else ports are wasted
        for (int i = 1; i < 6; i++)
        {
            bstring pcopy = bformat("mov     p%d.b, p0.b", i);
            bstrListAdd(code, pcopy);
            bdestroy(pcopy);
        }
    }

    bstring nameline = bformat("%s:", bdata(loopname));
    bstrListAdd(code, nameline);
    bdestroy(nameline);

    //bstrListAddChar(code, "\n");
    return 0;
}

int loopfooter(struct bstrList* code, bstring loopname, bstring loopreg, bstring condreg, bstring cond, bstring dir, bstring step)
//int loopfooter(struct bstrList* code, char* loopname, int step)
{
    struct tagbstring bsve = bsStatic("sve");
    //bstring bloop = bfromcstr(loopname);
    int sve = is_sve_bstr(loopname);

    bstring jumpline;
    bstring cmpline;
    if (sve)
    {
        //bstring bstep = bformat("incd GPR6", step);
        bstring bstep = bformat("incd %s", bdata(loopreg)); // auto VL on sve usage
        bstrListAdd(code, bstep);
        bdestroy(bstep);

        //bstrListAddChar(code, "whilelo  p0.d, GPR6, ARG1");
        jumpline = bformat("bne %s", bdata(loopname));
        cmpline = bformat("whilelo  p0.d, %s, %s", bdata(loopreg), bdata(condreg));
    }
    else
    {
        //bstring bstep = bformat("add GPR6, GPR6, #%d", step);
        bstring bstep = bformat("add %s, %s, #%s", bdata(loopreg), bdata(loopreg), bdata(step));
        bstrListAdd(code, bstep);
        bdestroy(bstep);

        //bstrListAddChar(code, "cmp   GPR6, ARG1");
        cmpline = bformat("cmp %s, %s", bdata(loopreg), bdata(condreg));
        jumpline = bformat("blt %s", bdata(loopname));
    }
    bstrListAdd(code, cmpline);
    bdestroy(cmpline);
    bstrListAdd(code, jumpline);
    bdestroy(jumpline);

    //bstrListAddChar(code, "\n");

    return 0;
}


struct tagbstring Registers[] = {
    bsStatic("x1"),
    bsStatic("x2"),
    bsStatic("x3"),
    bsStatic("x4"),
    bsStatic("x5"),
    bsStatic("x6"),
    bsStatic("x7"),
    bsStatic("x8"),
    bsStatic("x9"),
    bsStatic("x10"),
    bsStatic("x11"),
    bsStatic("x12"),
    bsStatic("x13"),
    bsStatic("x14"),
    bsStatic("x15"),
    bsStatic("x16"),
    bsStatic("x17"),
    bsStatic("x18"),
    bsStatic("x19"),
    bsStatic("x20"),
    bsStatic("x21"),
    bsStatic("x22"),
    bsStatic("x23"),
    bsStatic("x24"),
    bsStatic("x25"),
    bsStatic("x26"),
    bsStatic("x27"),
    bsStatic("x28"),
    bsStatic("")
};

struct tagbstring RegisterSptr = bsStatic("sp"); // x31, zero or streaming pointer
struct tagbstring RegisterBptr = bsStatic("x29"); // frame pointer
struct tagbstring RegisterLptr = bsStatic("x30"); // link register

#endif /* LIKWID_BENCH_ISA_ARMV8_H */
