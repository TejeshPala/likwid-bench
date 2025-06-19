/*
 * =======================================================================================
 *      Filename:  isa_armv7.h
 *
 *      Description:  Definitions used for dynamically compile benchmarks for ARMv7 systems
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
#ifndef LIKWID_BENCH_ISA_ARMV7_H
#define LIKWID_BENCH_ISA_ARMV7_H

#include "bstrlib.h"
#include "bstrlib_helper.h"
#include "test_strings.h"

#define ARCHNAME "armv7"
#define WORDLENGTH 4

int header(struct bstrList* code, bstring funcname)
{
    bstring glline = bformat(".global %s", bdata(funcname));
    bstring typeline = bformat(".type %s, \%function", bdata(funcname));
    bstring label = bformat("%s :", bdata(funcname));

    bstrListAddChar(code, ".cpu    cortex-a15\n.fpu    neon-vfpv4");
    bstrListAddChar(code, ".data");
    bstrListAddChar(code, ".text");
    bstrListAdd(code, glline);
    bstrListAdd(code, typeline);
    bstrListAdd(code, label);
    bstrListAddChar(code, "push     {r4-r7, lr}");
    bstrListAddChar(code, "add      r7, sp, #12");
    bstrListAddChar(code, "push     {r8, r10, r11}");
    bstrListAddChar(code, "vstmdb   sp!, {d8-d15}");

    bstrListAddChar(code, "\n");

    bstring streamline = bformat("%s", bdata(&bstrptr));
    bstrListAdd(code, streamline);
    bdestroy(streamline);

    bdestroy(glline);
    bdestroy(typeline);
    bdestroy(label);
    return 0;
}

int footer(struct bstrList* code, bstring funcname)
{
    bstrListAddChar(code, "vldmia   sp!, {d8-d15}");
    bstrListAddChar(code, "pop      {r8, r10, r11}");
    bstrListAddChar(code, "pop      {r4-r7, pc}");

    bstring line = bformat(".size %s, .-%s", bdata(funcname), bdata(funcname));
    bstrListAdd(code, line);
    bdestroy(line);

    bstrListAddChar(code, "\n");

    bstrListAddChar(code, "#if defined(__linux__) && defined(__ELF__)");
    bstrListAddChar(code, ".section .note.GNU-stack,\"\",%progbits");
    bstrListAddChar(code, "#endif");

    
}

int loopheader(struct bstrList* code, bstring loopname, bstring loopreg, bstring start, bstring condreg, bstring cond, bstring step)
//int loopheader(struct bstrList* code, char* loopname, int step)
{
    bstring line;
    if (loopname)
    {
        line = bformat("%s:", loopname);
    }
    else
    {
        line = bformat("kernelfunctionloop:");
    }

    // bstrListAddChar(code, "mov   GPR4, #0");
    bstring initline = bformat("mov %s, #%s", bdata(loopreg), bdata(start));
    bstrListAdd(code, initline);
    bdestroy(initline);

    bstring condline = bformat("mov %s, #%s", bdata(condreg), bdata(cond));
    bstrListAdd(code, condline);
    bdestroy(condline);

    bstrListAddChar(code, ".align 2");
    bstrListAdd(code, line);
    bstrListAddChar(code, "\n");

    bdestroy(line);
    return 0;
}

int loopfooter(struct bstrList* code, char* loopname, int step)
{
    bstring line;
    if (loopname)
    {
        line = bformat("blt %sb", loopname);
    }
    else
    {
        line = bformat("blt kernelfunctionloopb");
    }
    bstring bstep = bformat("add GPR4, #%d", step);
    bstrListAdd(code, bstep);
    bdestroy(bstep);
    bstrListAddChar(code, "cmp GPR4, GPR1");
    bstrListAdd(code, line);

    bstrListAddChar(code, "\n");

    bdestroy(line);
    return 0;
}


struct tagbstring  Registers[] = {
    bsStatic("r0"),
    bsStatic("r1"),
    bsStatic("r2"),
    bsStatic("r3"),
    bsStatic("r4"),
    bsStatic("r5"),
    bsStatic("r6"),
    bsStatic("r7"),
    bsStatic("r8"),
    bsStatic("r9"),
    bsStatic("r10"),
    bsStatic("r11"),
    bsStatic("r12"),
    bsStatic("r13"),
    bsStatic("r14"),
    bsStatic("r15"),
    bsStatic("")
};

struct tagbstring  RegisterSptr = bsStatic("sp");
struct tagbstring  RegisterBptr = bsStatic("rbp");

#endif /* LIKWID_BENCH_ISA_ARMV7_H */
