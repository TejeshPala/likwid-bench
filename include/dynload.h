#ifndef DYNLOAD_H
#define DYNLOAD_H

#include "bstrlib.h"
#include "test_types.h"

#define NUM_COMPILER_CANDIDATES 4
static struct tagbstring compiler_candidates[NUM_COMPILER_CANDIDATES] = {
    bsStatic("gcc"),
    bsStatic("icc"),
    bsStatic("icx"),
    bsStatic("clang"),
};

bstring get_compiler(bstring candidates);
int open_function(RuntimeWorkgroupConfig *wcfg);
int close_function(RuntimeWorkgroupConfig *wcfg);
int dump_assembly(RuntimeConfig *runcfg, bstring outfile);
int dynload_create_runtime_test_config(RuntimeConfig* rcfg, RuntimeWorkgroupConfig* wcfg);

#endif /* DYNLOAD_H */
