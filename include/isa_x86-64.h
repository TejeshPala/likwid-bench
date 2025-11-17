/*
 * =======================================================================================
 *      Filename:  isa_x86-64.h
 *
 *      Description:  Definitions used for dynamically compile benchmarks for x86-64 systems
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
#ifndef LIKWID_BENCH_ISA_X8664_H
#define LIKWID_BENCH_ISA_X8664_H

#include "bstrlib.h"
#include "bstrlib_helper.h"
#include "test_strings.h"

#define ARCHNAME "x86-64"


int header(struct bstrList* code, bstring funcname)
{
    bstring glline = bformat(".global %s", bdata(funcname));
    bstring typeline = bformat(".type %s, @function", bdata(funcname));
    bstring label = bformat("%s :", bdata(funcname));


    bstrListAddChar(code, ".intel_syntax noprefix");
    bstrListAddChar(code, ".data");
    bstrListAddChar(code, ".align 64\nSCALAR:\n.double 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0");
    bstrListAddChar(code, ".align 64\nSSCALAR:\n.single 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0");
    bstrListAddChar(code, ".align 64\nISCALAR:\n.int 1, 1, 1, 1, 1, 1, 1, 1");
    bstrListAddChar(code, ".align 16\nOMM:\n.int 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15");
    bstrListAddChar(code, ".align 16\nIOMM:\n.int 0,16,32,48,64,80,96,128,144,160,176,192,208,224,240,256");
    bstrListAddChar(code, ".align 16\nTOMM:\n.int 0,2,4,6,16,18,20,22,32,34,36,38,48,50,52,54");
    bstrListAddChar(code, ".text");
    bstrListAdd(code, glline);
    bstrListAdd(code, typeline);
    bstrListAdd(code, label);
    bstrListAddChar(code, "push rbp");
    bstrListAddChar(code, "mov rbp, rsp");
    bstrListAddChar(code, "push rbx");
    bstrListAddChar(code, "push r12");
    bstrListAddChar(code, "push r13");
    bstrListAddChar(code, "push r14");
    bstrListAddChar(code, "push r15");

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
    bstring line = bformat(".size %s, .-%s", bdata(funcname), bdata(funcname));
    bstrListAddChar(code, "pop r15");
    bstrListAddChar(code, "pop r14");
    bstrListAddChar(code, "pop r13");
    bstrListAddChar(code, "pop r12");
    bstrListAddChar(code, "pop rbx");
    bstrListAddChar(code, "mov  rsp, rbp");
    bstrListAddChar(code, "pop rbp");
    bstrListAddChar(code, "ret");

    bstrListAdd(code, line);

    bstrListAddChar(code, "\n");

    bstrListAddChar(code, "#if defined(__linux__) && defined(__ELF__)");
    bstrListAddChar(code, ".section .note.GNU-stack,\"\",%progbits");
    bstrListAddChar(code, "#endif");

    bdestroy(line);
}

int loopheader(struct bstrList* code, bstring loopname, bstring loopreg, bstring start, bstring condreg, bstring cond, bstring step)
{
    struct tagbstring bzero = bsStatic("0");
    struct tagbstring bplus = bsStatic("+");
    struct tagbstring bminus = bsStatic("-");

    bstring initline;
    if (bstrncmp(start, &bzero, blength(&bzero)) == BSTR_OK)
        initline = bformat("xor %s, %s", bdata(loopreg), bdata(loopreg));
    else
        initline = bformat("mov %s, %s", bdata(loopreg), bdata(start));
    bstrListAdd(code, initline);
    bdestroy(initline);

    bstring condline;
    int pos = 0;
    if (bstrncmp(cond, &bzero, blength(&bzero)) == BSTR_OK)
    {
        condline = bformat("xor %s, %s", bdata(condreg), bdata(condreg));
        bstrListAdd(code, condline);
    }
    else if ( (pos = binstr(cond, 0, &bplus)) > 0)
    {
        bstring base = bmidstr(cond, 0, pos);
        bstring offset = bmidstr(cond, pos + 1, blength(cond) - pos - 1);
        condline = bformat("mov %s, #%s", bdata(condreg), bdata(base));
        bstrListAdd(code, condline);
        condline = bformat("add %s, %s", bdata(condreg), bdata(offset));
        bstrListAdd(code, condline);
        bdestroy(base);
        bdestroy(offset);
    }
    else if ( (pos = binstr(cond, 0, &bminus)) > 0)
    {
        bstring base = bmidstr(cond, 0, pos);
        bstring offset = bmidstr(cond, pos + 1, blength(cond) - pos - 1);
        condline = bformat("mov %s, #%s", bdata(condreg), bdata(base));
        bstrListAdd(code, condline);
        condline = bformat("sub %s, %s", bdata(condreg), bdata(offset));
        bstrListAdd(code, condline);
        bdestroy(base);
        bdestroy(offset);
    }
    else
    {
        condline = bformat("mov %s, #%s", bdata(condreg), bdata(cond));
        bstrListAdd(code, condline);
    }
    bdestroy(condline);

    bstrListAddChar(code, ".align 16");

    bstring nameline = bformat("%s:", bdata(loopname));
    bstrListAdd(code, nameline);
    bdestroy(nameline);
    
    return 0;
}

int loopfooter(struct bstrList* code, bstring loopname, bstring loopreg, bstring condreg, bstring cond, bstring dir, bstring step)
{
    struct tagbstring bless = bsStatic("<");
    struct tagbstring blesseq = bsStatic("<=");
    struct tagbstring bgreat = bsStatic(">");
    struct tagbstring bgreateq = bsStatic(">=");
    struct tagbstring bequal = bsStatic("==");
    struct tagbstring bnequal = bsStatic("!=");

    bstring bstep = bformat("add %s, %s", bdata(loopreg), bdata(step));
    bstrListAdd(code, bstep);
    bdestroy(bstep);
    
    bstring bcmp = bformat("cmp %s, %s", bdata(loopreg), bdata(condreg));
    bstrListAdd(code, bcmp);
    bdestroy(bcmp);

    bstring jumpline;
    if (bstrncmp(dir, &bless, blength(&bless)) == BSTR_OK)
    {
        jumpline = bformat("jl %s", bdata(loopname));
    }
    else if (bstrncmp(dir, &blesseq, blength(&blesseq)) == BSTR_OK)
    {
        jumpline = bformat("jle %s", bdata(loopname));
    }
    else if (bstrncmp(dir, &bgreat, blength(&bgreat)) == BSTR_OK)
    {
        jumpline = bformat("jg %s", bdata(loopname));
    }
    else if (bstrncmp(dir, &bgreateq, blength(&bgreateq)) == BSTR_OK)
    {
        jumpline = bformat("jge %s", bdata(loopname));
    }
    else if (bstrncmp(dir, &bequal, blength(&bequal)) == BSTR_OK)
    {
        jumpline = bformat("je %s", bdata(loopname));
    }
    else if (bstrncmp(dir, &bnequal, blength(&bnequal)) == BSTR_OK)
    {
        jumpline = bformat("jne %s", bdata(loopname));
    }
    bstrListAdd(code, jumpline);
    bdestroy(jumpline);

    //bstrListAddChar(code, "\n");

    
    return 0;
}

int streamheader(struct bstrList* code, bstring reg, bstring ptr)
{
    bstring line = bformat("mov %s, %s", bdata(reg), bdata(ptr));
    bstrListAdd(code, line);
    bdestroy(line);

    return 0;
}

struct tagbstring Registers[] = {
    bsStatic("rax"),
    bsStatic("rbx"),
    bsStatic("rcx"),
    bsStatic("rdx"),
    bsStatic("rsi"),
    bsStatic("rdi"),
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

struct tagbstring RegisterSptr = bsStatic("rsp");
struct tagbstring RegisterBptr = bsStatic("rbp");

#endif /* LIKWID_BENCH_ISA_X8664_H */
