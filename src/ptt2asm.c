/*
 * =======================================================================================
 *
 *      Filename:  ptt2asm.c
 *
 *      Description:  Translating ptt file to object code
 *
 *      Version:   <VERSION>
 *      Released:  <DATE>
 *
 *      Author:   Thomas Gruber (tg), thomas.gruber@googlemail.com
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

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <stdint.h>

#include "map.h"
#include "bstrlib.h"
#include "bstrlib_helper.h"
#include "error.h"
#include "allocator.h"

#include "ptt2asm.h"



#ifdef __x86_64
#include "isa_x86-64.h"
#endif
#ifdef __i386__
#include "isa_x86.h"
#endif
#ifdef __ARM_ARCH_7A__
#include "isa_armv7.h"
#endif
#ifdef __ARM_ARCH_8A
#include "isa_armv8.h"
#endif
#ifdef _ARCH_PPC
#include "isa_ppc64.h"
#endif



static struct bstrList* bsplittrim(bstring str, const char c)
{
    struct bstrList* list = bsplit(str, c);
    if (list)
    {
        for (int i = 0; i < list->qty; i++)
        {
            btrimws(list->entry[i]);
        }
    }
    return list;
}

static struct bstrList* get_argList(bstring str)
{
    int obracket = -1;
    int ebracket = -1;
    obracket = bstrchrp(str, '(', 0);
    if (obracket == BSTR_ERR)
    {
        ERROR_PRINT("No opening bracket in string %s", bdata(str));
        return bstrListCreate();
    }
    ebracket = bstrchrp(str, ')', obracket);
    if (ebracket == BSTR_ERR)
    {
        ERROR_PRINT("No closing bracket in string %s", bdata(str));
        return bstrListCreate();
    }
    bstring s = bmidstr(str, obracket+1, ebracket - obracket - 1);
    btrimws(s);
    struct bstrList* list = bsplittrim(s, ',');
    bdestroy(s);
    return list;
}

static bstring get_name(bstring str)
{
    int obracket = -1;
    int firstcomma = -1;
    obracket = bstrchrp(str, '(', 0);
    if (obracket == BSTR_ERR)
    {
        ERROR_PRINT("No opening bracket in string %s", bdata(str));
        return bfromcstr("");
    }
    firstcomma = bstrchrp(str, ',', obracket);
    if (firstcomma == BSTR_ERR)
    {
        firstcomma = bstrchrp(str, ')', obracket);
        if (firstcomma == BSTR_ERR)
        {
            ERROR_PRINT("No comma-after-opening or closing bracket in string %s", bdata(str));
            return bfromcstr("");
        }
    }
    bstring s = bmidstr(str, obracket+1, firstcomma - obracket - 1);
    btrimws(s);
    return s;
}

static int has_prefix(bstring str, bstring prefix)
{
    return ((bstrncmp(str, prefix, blength(prefix)) == BSTR_OK) && (bchar(str, blength(prefix)) == '('));
}

static int toComment(struct bstrList* inout, bstring str)
{
    bstring comment = bformat("# %s", bdata(str));
    bstrListAdd(inout, comment);
    bdestroy(comment);
}

#include "ptt_keyword_loop.h"
#include "ptt_keyword_dummy.h"


static PttKeywordDefinition ptt_keys[] = {
    {.begin = "LOOP", .end = "LOOPEND", .parse = parse_loop},
    {.begin = "DUMMY", .end = "DUMMYEND", .parse = parse_dummy},
    // Must be last line
    {.begin = NULL, .end = NULL, .parse = NULL}
};


static int write_asm(bstring filename, struct bstrList* code)
{
    FILE* fp = NULL;
    char newline = '\n';
    size_t (*ownfwrite)(const void *ptr, size_t size, size_t nmemb, FILE *stream) = &fwrite;
    fp = fopen(bdata(filename), "w");
    if (fp)
    {
        for (int i = 0; i < code->qty; i++)
        {
            ownfwrite(bdata(code->entry[i]), 1, blength(code->entry[i]), fp);
            ownfwrite(&newline, 1, sizeof(char), fp);
        }
        fclose(fp);
        return 0;
    }
    return 1;
}


static int _analyse_keyword(TestConfig_t config, struct bstrList* openkeys, struct bstrList* closekeys, struct bstrList* code, struct bstrList* out)
{
    int i = 0;
    int line = 0;
    int keyIdx = -1;
    int analyzeKey = -1;
    int postKey = 0;
    int openingKeyLine = -1;
    int closingKeyLine = -1;
    int err = 0;
    // Store the name/label of the keyword
    bstring openname;
    // Record lines prior to keywords
    struct bstrList* body = bstrListCreate();
    // For the lines between the keyword tags including the tag lines
    struct bstrList* pre = bstrListCreate();
    // Record lines after the keywords
    struct bstrList* post = bstrListCreate();

    // Iterate over code to split it in three parts:
    // - pre : before the first keyword
    // - body: lines between the keyword tags including the tag lines
    // - pre: after the closing keyword 
    while (line < code->qty)
    {
        // No keyword found yet, to check whether the line contains a keyword
        if (keyIdx < 0 && analyzeKey < 0)
        {
            // Check keyword beginnings using the pre-allocated list
            for (i = 0; i < openkeys->qty; i++)
            {
                if (has_prefix(code->entry[line], openkeys->entry[i]))
                {
                    // There is a keyword, get the name/label to find the related keyword closing
                    openname = get_name(code->entry[line]);
                    // Add opening keyword line to body
                    bstrListAdd(body, code->entry[line]);
                    DEBUG_PRINT(DEBUGLEV_DETAIL, "Opening keyword '%s' for label '%s' in line %d", bdata(openkeys->entry[i]), bdata(openname), line);
                    // Store index in keyword list for matching the corresponding closing keyword
                    keyIdx = i;
                    // Store line index with opening keyword
                    openingKeyLine = line;
                    break;
                }
            }
            // There is a valid keyword, go to next line
            if (keyIdx >= 0)
            {
                line++;
                continue;
            }
        }
        // There was already a beginning keyword, so check for the related closing keyword
        if (keyIdx >= 0 && analyzeKey < 0)
        {
            // Independent whether this is the line with the right keyword closing and name, the line is required in the analysis input
            bstrListAdd(body, code->entry[line]);
            // Do we have a closing keyword?
            if (has_prefix(code->entry[line], closekeys->entry[keyIdx]))
            {
                // Check for the right keyword name
                bstring closename = get_name(code->entry[line]);
                if (bstrcmp(openname, closename) == BSTR_OK)
                {
                    // This is the related closing keyword to our opening
                    DEBUG_PRINT(DEBUGLEV_DETAIL, "Closing keyword '%s' for label '%s' in line %d", bdata(closekeys->entry[keyIdx]), bdata(closename), line);
                    // Store the keyword index for analysis
                    analyzeKey = keyIdx;
                    // This is used only inside the loop and we are leaving the keyword area
                    keyIdx = -1;
                    // Every line after this one is a post-keyword line
                    postKey = 1;
                    // Store line index with closing keyword
                    closingKeyLine = line;
                }
                bdestroy(closename);
            }
        }
        else
        {
            if (!postKey)
            {
                // This line is prior to any keyword
                bstrListAdd(pre, code->entry[line]);
            }
            else
            {
                // This line is after a found keyword area
                bstrListAdd(post, code->entry[line]);
            }
        }
        line++;
    }
    if (openingKeyLine >= 0 && closingKeyLine < 0)
    {
        DEBUG_PRINT(DEBUGLEV_INFO, "Closing keyword %s(%s ...) missing", bdata(closekeys->entry[keyIdx]), bdata(openname));
        return -EINVAL;
    }
    if (openingKeyLine < 0 && closingKeyLine >= 0)
    {
        DEBUG_PRINT(DEBUGLEV_INFO, "Closing keyword but corresponding opening keyword is missing");
        return -EINVAL;
    }

    // Check if there is a keyword for analysis in the code
    if (analyzeKey >= 0 && openingKeyLine >= 0 && closingKeyLine >= 1)
    {
        // First we parse the keyword area including beginning and closing keyword
        // The keyword lines contain inputs for the analysis
        struct bstrList* tmp = bstrListCreate();
        DEBUG_PRINT(DEBUGLEV_DETAIL, "Parsing keyword '%s .. %s'", bdata(openkeys->entry[analyzeKey]), bdata(closekeys->entry[analyzeKey]));
        err = ptt_keys[analyzeKey].parse(config, body, tmp);
        if (err == 0)
        {
            // The output of the analysis is used as input for a recursive checking for nested keywords
            struct bstrList* tmpout = bstrListCreate();
            DEBUG_PRINT(DEBUGLEV_DETAIL, "Recursively check analysis results for nested keywords");
            err = _analyse_keyword(config, openkeys, closekeys, tmp, tmpout);
            if (err == 0)
            {
                // The lines returned by the recursion form our new keyword area
                // Delete body
                bstrListDestroy(body);
                // Re-create body with analyzed lines
                body = bstrListCopy(tmpout);
            }
            bstrListDestroy(tmpout);
        }
        bstrListDestroy(tmp);
        // The name/label of the opening keyword is only set if there is an analysis key
        bdestroy(openname);
    }

    // Assembly the output
    // If there was a keyword, all three sections (pre, body, post) are filled
    // If there was NO keyword, all liney are in the pre section
    if (err == 0)
    {
        for (int i = 0; i < pre->qty; i++)
        {
            bstrListAdd(out, pre->entry[i]);
        }
        for (int i = 0; i < body->qty; i++)
        {
            bstrListAdd(out, body->entry[i]);
        }
        for (int i = 0; i < post->qty; i++)
        {
            bstrListAdd(out, post->entry[i]);
        }
    }

    // Cleanup used data structures
    bstrListDestroy(pre);
    bstrListDestroy(body);
    bstrListDestroy(post);
    
    return err;
}


int prepare_ptt(TestConfig_t config, struct bstrList* out, struct bstrList* regs)
{
    int i = 0;
    // Just for debugging
    int round = 0;
    bstring dstring;
    struct tagbstring sep = bsStatic(", ");

    // Check whether there is no code or no output list
    if (blength(config->code) == 0 || !out)
    {
        return -EINVAL;
    }

    // We need the code line-by-line for analysis (trims left and right whitespaces of each line)
    struct bstrList* code = bsplittrim(config->code, '\n');
    if ((!code) || (code->qty <= 0))
    {
        return -EINVAL;
    }

    // Create a list of all keyword beginnings and closings to avoid multiple recreation/frees
    struct bstrList* openkeys = bstrListCreate();
    struct bstrList* closekeys = bstrListCreate();
    i = 0;
    while (ptt_keys[i].begin != NULL)
    {
        bstrListAddChar(openkeys, ptt_keys[i].begin);
        bstrListAddChar(closekeys, ptt_keys[i].end);
        i++;
    }
    if (openkeys->qty != closekeys->qty)
    {
        ERROR_PRINT("Invalid keyword definition");
        bstrListDestroy(code);
        bstrListDestroy(openkeys);
        bstrListDestroy(closekeys);
        return -EINVAL;
    }

    struct bstrList* opennames = bstrListCreate();
    struct bstrList* closenames = bstrListCreate();
    for (i = 0; i < code->qty; i++)
    {
        for (int j = 0; j < openkeys->qty; j++)
        {
            if (has_prefix(code->entry[i], openkeys->entry[j]))
            {
                bstring n = get_name(code->entry[i]);
                bstrListAdd(opennames, n);
                bdestroy(n);
            }
        }
        for (int j = 0; j < closekeys->qty; j++)
        {
            if (has_prefix(code->entry[i], closekeys->entry[j]))
            {
                bstring n = get_name(code->entry[i]);
                bstrListAdd(closenames, n);
                bdestroy(n);
            }
        }
    }
    dstring = bjoin(opennames, &sep);
    DEBUG_PRINT(DEBUGLEV_DEVELOP, "Opening keyword labels: %s", bdata(dstring));
    bdestroy(dstring);
    dstring = bjoin(closenames, &sep);
    DEBUG_PRINT(DEBUGLEV_DEVELOP, "Closing keyword labels: %s", bdata(dstring));
    bdestroy(dstring);
    if (opennames->qty == 0 || closenames->qty == 0)
    {
        ERROR_PRINT("Code without keyword names");
        bstrListDestroy(code);
        bstrListDestroy(openkeys);
        bstrListDestroy(closekeys);
        bstrListDestroy(opennames);
        bstrListDestroy(closenames);
        return -EINVAL;
    }
    struct bstrList* bothnames = bstrListCreate();
    for (i = 0; i < opennames->qty; i++)
    {
        for (int j = 0; j < closenames->qty; j++)
        {
            if (bstrcmp(opennames->entry[i], closenames->entry[j]) == BSTR_OK)
            {
                bstrListAdd(bothnames, opennames->entry[i]);
            }
        }
    }
    if ((bothnames->qty != opennames->qty) || (bothnames->qty != closenames->qty))
    {
        ERROR_PRINT("Invalid label for keyword");
        bstrListDestroy(code);
        bstrListDestroy(openkeys);
        bstrListDestroy(closekeys);
        bstrListDestroy(opennames);
        bstrListDestroy(closenames);
        return -EINVAL;
    }
    bstrListDestroy(opennames);
    bstrListDestroy(closenames);
    bstrListDestroy(bothnames);

    // Copy input
    struct bstrList* pttin = bstrListCopy(code);
    // Create empty output for the loop
    struct bstrList* pttout = bstrListCreate();

    // This call analyses the code and recusively enters keyword surrounded areas for nested analysis
    // The do-while loop is used to also analyse inputs with multiple keywords after each other.
    int done = 0;
    do {
        DEBUG_PRINT(DEBUGLEV_DEVELOP, "Round %d", round);
        _analyse_keyword(config, openkeys, closekeys, pttin, pttout);
        // Check if there are still opening or closing keywords in code
        int found = 0;
        for (int i = 0; i < pttout->qty; i++)
        {
            // Any opening keywords?
            for (int j = 0; j < openkeys->qty; j++)
            {
                if (has_prefix(pttout->entry[i], openkeys->entry[j]))
                {
                    found = 1;
                    break;
                }
            }
            // Any closing keywords?
            // This is needed in case there is only a closing keyword left (invalid syntax)
            // This breaks in the next round
            for (int j = 0; j < closekeys->qty && !found; j++)
            {
                if (has_prefix(pttout->entry[i], closekeys->entry[j]))
                {
                    found = 1;
                    break;
                }
            }
            if (found) break;
        }

        for (int i = 0; i < pttin->qty; i++)
        {
            for (int j = 0; j < Registers[j].data[0] != '\0'; j++)
            {
                if (binstr(pttin->entry[i], 0, &Registers[j]) != BSTR_ERR)
                {
                    bstrListAdd(regs, &Registers[j]);
                }
            }
        }
        DEBUG_PRINT(DEBUGLEV_DEVELOP, "List of registers used before duplication removal");
        if (global_verbosity == DEBUGLEV_DEVELOP) bstrListPrint(regs);
        bstrListRemoveDup(regs);
        DEBUG_PRINT(DEBUGLEV_DEVELOP, "List of registers used after duplication removal");
        if (global_verbosity == DEBUGLEV_DEVELOP) bstrListPrint(regs);

        // Destroy input, not needed anymore
        bstrListDestroy(pttin);
        // Is there still a keyword?
        if (!found)
        {
            done = 1;
            DEBUG_PRINT(DEBUGLEV_DEVELOP, "No more keywords -> exit parsing");
        }
        else
        {
            DEBUG_PRINT(DEBUGLEV_DEVELOP, "Still keywords in code -> new round");
            // Last output is new input
            pttin = pttout;
            // Fresh list for next round's output
            pttout = bstrListCreate();
        }
        round++;
    } while (!done);

    // Add final parsing result to output list
    for (int i = 0; i < pttout->qty; i++)
    {
        bstrListAdd(out, pttout->entry[i]);
    }

    // Cleanup
    bstrListDestroy(code);
    bstrListDestroy(openkeys);
    bstrListDestroy(closekeys);
    bstrListDestroy(pttout);
    return 0;
}

static int _generate_replacement_lists(RuntimeConfig* runcfg, RuntimeThreadConfig* thread, struct bstrList* keys, struct bstrList* values, int* maxKeyLength, struct bstrList* regsused)
{
    TestConfig_t config = runcfg->tcfg;
    int maxs = 0;
    struct bstrList* regsavail = bstrListCreate();
    struct tagbstring bstrptr = bsStatic("# STREAMPTRFORREPLACMENT");
    struct tagbstring bnewline = bsStatic("\n");
    bstrListAddChar(keys, "NAME");
    bstrListAdd(values, config->name);

    for (int r = 0; Registers[r].data[0] != '\0'; r++)
    {
        bstrListAdd(regsavail, &Registers[r]);
    }
    for (int i = 0; i < regsused->qty; i++)
    {
        for (int r = 0; r < regsavail->qty; r++)
        {
            if (bstrncmp(&Registers[r], regsused->entry[i], blength(&Registers[r])) == BSTR_OK)
            {
                bstrListRemove(regsavail, &Registers[r]);
            }
        }
    }
    DEBUG_PRINT(DEBUGLEV_DEVELOP, "List of registers available");
    if (global_verbosity == DEBUGLEV_DEVELOP) bstrListPrint(regsavail);


    // Runtime parameters missing
    // - STREAM data
    // - Infos from the command line parameters
    // - Infos about threading
    struct bstrList* blines = bstrListCreate();
    int str_count = 0;
    if (thread->num_streams > regsavail->qty)
    {
        ERROR_PRINT("The Number of streams %d is higher than the %d registers available", thread->num_streams, regsavail->qty);
        bstrListDestroy(regsavail);
        bstrListDestroy(blines);
        return -EINVAL;
    }
    for (int s = 0; s < thread->num_streams; s++)
    {
        RuntimeStreamConfig* data = &thread->sdata[s];
        RuntimeThreadStreamConfig* str = &thread->tstreams[s];
        for (int i = 0; i < regsavail->qty && data->name; i++)
        {
            str_count++;
            bstrListAdd(keys, data->name);
            bstring reg = bformat("%s", bdata(regsavail->entry[i]));
            bstring ptr = bformat("%p", str->tstream_ptr);
            bstrListAdd(values, reg);
#if defined(__x86_64) || defined(__x86_64__)
            bstring line = bformat("mov %s, %s", bdata(regsavail->entry[i]), bdata(ptr));
#elif defined(__ARM_ARCH_8A) || defined(__aarch64__) || defined(__arm__)
            bstring line = bformat("ldr %s, =%s", bdata(regsavail->entry[i]), bdata(ptr));
#elif defined(_ARCH_PPC) || defined(__powerpc) || defined(__ppc__) || defined(__PPC__)
            bstring line = bformat("mov %s, %s", bdata(regsavail->entry[i]), bdata(ptr)); // to be replaced
#endif
            bstrListAdd(blines, line);
            if (bstrListRemove(regsavail, reg) == BSTR_OK)
            {
                DEBUG_PRINT(DEBUGLEV_DEVELOP, "Register '%s' removed: List of registers available", bdata(reg));
                if (global_verbosity == DEBUGLEV_DEVELOP) bstrListPrint(regsavail);
            }
            bdestroy(reg);
            bdestroy(ptr);
            bdestroy(line);
            break;
        }

        TestConfigStream *tstr = &runcfg->tcfg->streams[s];
        for (int d = 0; d < data->dims; d++)
        {
            bstring k = bformat("#%s", bdata(tstr->dims->entry[d]));
            bstring v = bformat("%" PRIu64, str->tsizes[d]);
            bstrListAdd(keys, k);
            bstrListAdd(values, v);
            bdestroy(k);
            bdestroy(v);
        }
    }
    if (str_count > 0)
    {
        bstrListAdd(keys, &bstrptr);
        bstring val = bjoin(blines, &bnewline);
        bstrListAdd(values, val);
        bdestroy(val);
        DEBUG_PRINT(DEBUGLEV_DEVELOP, "The streams line '%s' for replacement is", bdata(&bstrptr))
        if (global_verbosity == DEBUGLEV_DEVELOP) bstrListPrint(blines);
    }
    bstrListDestroy(blines);

    for (int i = 0; i < config->num_constants; i++)
    {
        TestConfigVariable *var = &config->constants[i];
        bstrListAdd(keys, var->name);
        bstrListAdd(values, var->value);
    }
    for (int i = 0; i < config->num_vars; i++)
    {
        TestConfigVariable *var = &config->vars[i];
        bstrListAdd(keys, var->name);
        bstrListAdd(values, var->value);
    }
    // Get the max key length
    // In the replacement step, we iterate over the list and start with the longest keys
    // The reason is that short strings are likely substrings of others, like N (array length) and
    // NAME (test name). If we start with N, it will replace the N in NAME and the later replacement
    // of NAME fails.
    for (int i = 0; i < keys->qty; i++)
    {
        int m = blength(keys->entry[i]);
        if (m > maxs) maxs = m;
    }
    *maxKeyLength = maxs;
    bstrListDestroy(regsavail);
    return 0;
}

int generate_code(RuntimeConfig* runcfg, RuntimeThreadConfig* thread, struct bstrList* out)
{
    TestConfig_t config = runcfg->tcfg;
    struct bstrList* tmp = bstrListCreate();
    struct bstrList* regsused = bstrListCreate();
    header(tmp, config->name);
    prepare_ptt(config, tmp, regsused);
    footer(tmp, config->name);

    // Now we have the function code but still with variables
    struct bstrList* keys = bstrListCreate();
    struct bstrList* values = bstrListCreate();
    int maxLength = 0;
    int err = _generate_replacement_lists(runcfg, thread, keys, values, &maxLength, regsused);
    
    for (int i = 0; i < tmp->qty; i++)
    {
        for (int l = maxLength; l >= 1; l--)
        {
            for (int j = 0; j < keys->qty; j++)
            {
                if (blength(keys->entry[j]) == l && binstr(tmp->entry[i], 0, keys->entry[j]) != BSTR_ERR)
                {
                    DEBUG_PRINT(DEBUGLEV_DEVELOP, "Replacing '%s' with '%s' in '%s'", bdata(keys->entry[j]), bdata(values->entry[j]), bdata(tmp->entry[i]));
                    bfindreplace(tmp->entry[i], keys->entry[j], values->entry[j], 0);
                }
            }
        }
    }
    bstrListDestroy(keys);
    bstrListDestroy(values);

    // Add final generated result to output list
    for (int i = 0; i < tmp->qty; i++)
    {
        bstrListAdd(out, tmp->entry[i]);
    }
    bstrListDestroy(tmp);
    bstrListDestroy(regsused);
    return 0;
}
