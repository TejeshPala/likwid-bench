/*
 * =======================================================================================
 *
 *      Filename:  ptt_keyword_dummy.h
 *
 *      Description:  Header file for the keyword DUMMY & DUMMYEND
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

#ifndef PTT_KEYWORD_DUMMY_H
#define PTT_KEYWORD_DUMMY_H

#include "test_types.h"
#include "bstrlib.h"
#include "bstrlib_helper.h"
#include "error.h"

int parse_dummy(TestConfig_t config, struct bstrList* code, struct bstrList* out)
{
    int err = 0;
    struct tagbstring bkeybegin = bsStatic("DUMMY");
    struct tagbstring bkeyend = bsStatic("DUMMYEND");
    for (int i = 0; i < code->qty; i++)
    {
        DEBUG_PRINT(DEBUGLEV_DEVELOP, "DUMMY: %s", bdata(code->entry[i]));
    }
    // First we do some basic checks
    // Does the first line start with 'DUMMY'
    if (!has_prefix(code->entry[0], &bkeybegin))
    {
        ERROR_PRINT("First line does not start with %s", bdata(&bkeybegin));
        return -EINVAL;
    }
    // Does the last line start with 'DUMMYEND'
    if (!has_prefix(code->entry[code->qty-1], &bkeyend))
    {
        ERROR_PRINT("Last line does not start with %s", bdata(&bkeyend));
        return -EINVAL;
    }
    // Now get a list of all arguments in the first line -> DUMMY(arg0, arg1, ...)
    struct bstrList* beginArgs = get_argList(code->entry[0]);
    if (!beginArgs)
    {
        ERROR_PRINT("Arguments missing in %s line: %s", bdata(&bkeybegin), bdata(code->entry[0]));
        return -EINVAL;
    }
    // The DUMMY line should have just a single argument
    if (beginArgs->qty != 1)
    {
        ERROR_PRINT("Arguments missing in %s line: %s", bdata(&bkeybegin), bdata(code->entry[0]));
        bstrListDestroy(beginArgs);
        return -EINVAL;
    }

    // Now get a list of all arguments in the last line -> DUMMYEND(arg0, arg1, ...)
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

    // This dummy keyword just copies the text
    toComment(out, code->entry[0]);
    for (int i = 1; i < code->qty-1; i++)
    {
        bstrListAdd(out, code->entry[i]);
    }
    toComment(out, code->entry[code->qty-1]);
    // Cleaup used data structures
    bstrListDestroy(beginArgs);
    bstrListDestroy(endArgs);
    return 0;
}


#endif /* PTT_KEYWORD_DUMMY_H */
