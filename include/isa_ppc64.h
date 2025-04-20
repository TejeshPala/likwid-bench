/*
 * =======================================================================================
 *      Filename:  isa_ppc64.h
 *
 *      Description:  Definitions used for dynamically compile benchmarks for POWER systems
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
#ifndef LIKWID_BENCH_ISA_PPC64_H
#define LIKWID_BENCH_ISA_PPC64_H


#include "bstrlib.h"
#include "bstrlib_helper.h"
#include "test_strings.h"

#define ARCHNAME "ppc64"
#define WORDLENGTH 4


int header(struct bstrList* code, bstring funcname)
{
    bstring glline = bformat(".globl %s", bdata(funcname));
    bstring typeline = bformat(".type %s, @function", bdata(funcname));
    bstring label = bformat("%s :", bdata(funcname));
    bstring Llabel = bformat(".L.%s:", bdata(funcname));
    bstring localentry = bformat(".localentry %s, .-%s", bdata(funcname), bdata(funcname));

    bstrListAddChar(code, ".data");
    bstrListAddChar(code, ".align 64\nSCALAR:\n.double 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0");
    bstrListAddChar(code, ".align 64\nSSCALAR:\n.single 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0");
    bstrListAddChar(code, ".align 64\nISCALAR:\n.int 1, 1, 1, 1, 1, 1, 1, 1");
    bstrListAddChar(code, ".align 16\nOMM:\n.int 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15");
    bstrListAddChar(code, ".align 16\nIOMM:\n.int 0,16,32,48,64,80,96,128,144,160,176,192,208,224,240,256");
    bstrListAddChar(code, ".align 16\nTOMM:\n.int 0,2,4,6,16,18,20,22,32,34,36,38,48,50,52,54");
    bstrListAddChar(code, ".text");
    bstrListAddChar(code, ".set r0,0; .set SP,1; .set RTOC,2; .set r3,3; .set r4,4;");
    bstrListAddChar(code, ".set r5,5; .set r6,6; .set r7,7; .set r8,8; .set r9,9; .set r10,10;");
    bstrListAddChar(code, ".set r11,11; .set r12,12; .set r13,13; .set r14,14; .set r15,15; .set r16,16;");
    bstrListAddChar(code, ".set x0,0; .set x1,1; .set x2,2; .set x3,3; .set x4,4;");
    bstrListAddChar(code, ".set x5,5; .set x6,6; .set x7,7; .set x8,8; .set x9,9;");
    bstrListAddChar(code, ".set vec0,0; .set vec1,1; .set vec2,2; .set vec3,3;");
    bstrListAddChar(code, ".set vec4,4; .set vec5,5; .set vec6,6; .set vec7,7;");
    bstrListAddChar(code, ".set vec8,8; .set vec9,9; .set vec10,10; .set vec11,11;");
    bstrListAddChar(code, ".set vec12,12;");
    bstrListAddChar(code, ".abiversion 2");
    bstrListAddChar(code, ".section    \".toc\",\"aw\"");
    bstrListAddChar(code, ".section    \".text\"");
    bstrListAddChar(code, ".align 2");
    bstrListAdd(code, glline);
    bstrListAdd(code, typeline);
    bstrListAdd(code, label);
    bstrListAdd(code, Llabel);
    bstrListAdd(code, localentry);

    bstrListAddChar(code, "\n");

    bstring streamline = bformat("%s", bdata(&bstrptr));
    bstrListAdd(code, streamline);
    bdestroy(streamline);

    bdestroy(glline);
    bdestroy(typeline);
    bdestroy(label);
    bdestroy(Llabel);
    bdestroy(localentry);
    return 0;
}

int footer(struct bstrList* code, bstring funcname)
{
    bstrListAddChar(code, "blr");
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
    // In order to use the dedicated counter register of the POWER architecture, we
    // have to initialize it. It will decrement at each jump automatically and stop
    // jumping if zero.

    // Temporarly store the content of r12 into r0
    bstrListAddChar(code, "li r0, r12");

    // Load r12 with step count
    bstring bstep = bformat("li r12, %s", bdata(step));
    bstrListAdd(code, bstep);
    bdestroy(bstep);

    // Load loop length into condition register
    bstring bcond = bformat("li %s, %s", bdata(condreg), bdata(cond));
    bstrListAdd(code, bcond);
    bdestroy(bcond);

    // Devide condition by step count to get amount of needed jumps
    //bstrListAddChar(code, "divd r12, r3, r12");
    bstring bdivd = bformat("divd r12, %s, r12", bdata(condreg));
    bstrListAdd(code, bdivd);
    bdestroy(bdivd);

    // Load count into counter register
    bstrListAddChar(code, "mtctr r12");

    // Restore register content of r12 from r0
    bstrListAddChar(code, "li r12, r0");

    // Add loop label
    bstring line = bformat("%s:", bdata(loopname));
    bstrListAdd(code, line);
    bdestroy(line);

    bstrListAddChar(code, "\n");

    return 0;
}

int loopfooter(struct bstrList* code, bstring loopname, bstring loopreg, bstring condreg, bstring cond, bstring dir, bstring step)
//int loopfooter(struct bstrList* code, char* loopname, int step)
{
    //bstring bstep = bformat("addi 4, 4, %s", bdata(step));
    // Increment the loop register by step
    bstring bstep = bformat("addi %s, %s, %s", bdata(loopreg), bdata(loopreg), bdata(step));
    bstrListAdd(code, bstep);
    bdestroy(bstep);

    // There is no conditon checking between loopreg and condreg for POWER because
    // there is the dedicated counter register which decrements at each jump if not zero.
    bstring line = bformat("bdnz %sb", bdata(loopname));
    bstrListAdd(code, line);
    bdestroy(line);

    bstrListAddChar(code, "\n");

    return 0;
}


struct tagbstring  Registers[] = {
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
    bsStatic("r16"),
    bsStatic("17"),
    bsStatic("18"),
    bsStatic("19"),
    bsStatic("20"),
    bsStatic("21"),
    bsStatic("22"),
    bsStatic("23"),
    bsStatic("24"),
    bsStatic("25"),
    bsStatic("26"),
    bsStatic("27"),
    bsStatic("28"),
    bsStatic("29"),
    bsStatic("30"),
    bsStatic("31"),
    bsStatic("")
};

struct tagbstring  RegisterSptr = bsStatic("SP");
struct tagbstring  RegisterBptr = bsStatic("rbp");

#endif /* LIKWID_BENCH_ISA_PPC64_H */
