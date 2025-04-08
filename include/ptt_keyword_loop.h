/*
 * =======================================================================================
 *
 *      Filename:  ptt_keyword_loop.h
 *
 *      Description:  Header file for the keyword LOOP & LOOPEND
 *
 *      Version:   <VERSION>
 *      Released:  <DATE>
 *
 *      Author:   Thomas Gruber (tg), thomas.roehl@googlemail.com
 *      Project:  likwid-bench
 *
 *      Copyright (C) 2019 RRZE, University Erlangen-Nuremberg
 *
 *      This program is free software: you can redistribute it and/or modify it under
 *      the terms of the GNU General Public License as published by the Free Software
 *      Foundation, either version 2 of the License, or (at your option) any later
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

#ifndef PTT_KEYWORD_LOOP_H
#define PTT_KEYWORD_LOOP_H

#include "test_types.h"
#include "bstrlib.h"
#include "bstrlib_helper.h"

int parse_loop(TestConfig_t config, struct bstrList* code, struct bstrList* out)
{
    int err = 0;
    struct tagbstring bkeybegin = bsStatic("LOOP");
    struct tagbstring bkeyend = bsStatic("LOOPEND");
    for (int i = 0; i < code->qty; i++)
    {
        DEBUG_PRINT(DEBUGLEV_DEVELOP, "LOOP: %s", bdata(code->entry[i]));
    }
    // First we do some basic checks
    // Does the first line start with 'LOOP'
    if (!has_prefix(code->entry[0], &bkeybegin))
    {
        ERROR_PRINT("First line does not start with %s", bdata(&bkeybegin));
        return -EINVAL;
    }
    // Does the last line start with 'LOOPEND'
    if (!has_prefix(code->entry[code->qty-1], &bkeyend))
    {
        ERROR_PRINT("Last line does not start with %s", bdata(&bkeyend));
        return -EINVAL;
    }
    // Now get a list of all arguments in the first line -> LOOP(arg0, arg1, ...)
    struct bstrList* beginArgs = get_argList(code->entry[0]);
    if (!beginArgs)
    {
        ERROR_PRINT("Arguments missing in %s line: %s", bdata(&bkeybegin), bdata(code->entry[0]));
        return -EINVAL;
    }
    // The LOOP line should have 5 arguments
    if (beginArgs->qty != 5)
    {
        ERROR_PRINT("Arguments missing in %s line: %s", bdata(&bkeybegin), bdata(code->entry[0]));
        bstrListDestroy(beginArgs);
        return -EINVAL;
    }
    // Now get a list of all arguments in the last line -> LOOPEND(arg0, arg1, ...)
    struct bstrList* endArgs = get_argList(code->entry[code->qty-1]);
    if (!endArgs)
    {
        ERROR_PRINT("Arguments missing in %s line: %s", bdata(&bkeyend), bdata(code->entry[code->qty-1]));
        bstrListDestroy(beginArgs);
        return -EINVAL;
    }

    // Do both lines have the same first argument aka the keyword label
    if (bstrcmp(beginArgs->entry[0], endArgs->entry[0]) != BSTR_OK)
    {
        ERROR_PRINT("%s and %s for different loops %s <-> %s", bdata(&bkeybegin), bdata(&bkeyend), bdata(beginArgs->entry[0]), bdata(endArgs->entry[0]));
        bstrListDestroy(beginArgs);
        bstrListDestroy(endArgs);
        return -EINVAL;
    }
    // List of all LOOP arguments:
    // 0: loopname
    // 1: <loopreg>=<initvalue>
    // 2: loop direction > or <
    // 3: <condreg>=<endvalue>
    // 4: <increment>

    // The startloop and endloop are a set of <register>=<value> and need to be split
    struct bstrList* startloop = bsplittrim(beginArgs->entry[1], '=');
    struct bstrList* endloop = bsplittrim(beginArgs->entry[3], '=');
    // We check both conditions at one to avoid too much code
    if (startloop->qty != 2 || endloop->qty != 2)
    {
        ERROR_PRINT("Loop init (reg=value) or loop end (reg=value) invalid in LOOP line: %s", bdata(code->entry[0]));
        bstrListDestroy(startloop);
        bstrListDestroy(endloop);
        bstrListDestroy(beginArgs);
        bstrListDestroy(endArgs);
        return -EINVAL;
    }
    toComment(out, code->entry[0]);
    /* This creates the arch-specific assembly to setup a loop */
    loopheader(out, beginArgs->entry[0], startloop->entry[0], startloop->entry[1], endloop->entry[0], endloop->entry[1], beginArgs->entry[4]);
    // Add the loop body
    for (int i = 1; i < code->qty-1; i++)
    {
        bstrListAdd(out, code->entry[i]);
    }
    /* This creates the arch-specific assembly to close a loop (compare + jump commonly)*/
    toComment(out, code->entry[code->qty-1]);
    loopfooter(out, endArgs->entry[0], startloop->entry[0], endloop->entry[0], endloop->entry[1], beginArgs->entry[2], beginArgs->entry[4]);

    // Cleaup used data structures
    bstrListDestroy(startloop);
    bstrListDestroy(endloop);
    bstrListDestroy(beginArgs);
    bstrListDestroy(endArgs);
    return 0;
}


#endif /* PTT_KEYWORD_LOOP_H */
