#ifndef CLI_PARSER_H
#define CLI_PARSER_H

#include "bstrlib.h"

static struct option_extra baseopts[6] = {
    {"help", 'h', RETURN_TYPE_MASK(RETURN_TYPE_BOOL), 0, "Help and usage"},
    {"verbose", 'V', RETURN_TYPE_MASK(RETURN_TYPE_INT), required_argument, "Verbosity level (0 - 3)"},
    {"test", 't', RETURN_TYPE_MASK(RETURN_TYPE_BSTRING), required_argument, "Test case to run"},
    {"file", 'f', RETURN_TYPE_MASK(RETURN_TYPE_BSTRING), required_argument, "Test file run run", ARG_FLAG_MASK(ARG_FLAG_FILE)},
    {"kfolder", 'd', RETURN_TYPE_MASK(RETURN_TYPE_BSTRING), required_argument, "Directory with benchmarks", ARG_FLAG_MASK(ARG_FLAG_DIR)},
    {0, 0, 0, 0}
};

static struct option_extra basetestopts[4] = {
    {"iters", 'i', RETURN_TYPE_MASK(RETURN_TYPE_UINT64), required_argument, "Number of test case executions"},
    {"workgroup", 'w', RETURN_TYPE_MASK(RETURN_TYPE_BSTRING)|RETURN_TYPE_MASK(RETURN_TYPE_LIST), required_argument, "Workgroup definition (Threads + Streams)"},
    {"runtime", 'r', RETURN_TYPE_MASK(RETURN_TYPE_DOUBLE), required_argument, "Minimal runtime of a test case (use instead of -i/--iters)", ARG_FLAG_MASK(ARG_FLAG_TIME)},
    {0, 0, 0, 0}
};

struct parameter_option {
    struct tagbstring name;
    int arg_flags;
    int ret_flags;
};

static struct parameter_option param_opts[] = {
    {bsStatic("bytes"), ARG_FLAG_MASK(ARG_FLAG_BYTES), RETURN_TYPE_MASK(RETURN_TYPE_UINT64)},
    {bsStatic("time"), ARG_FLAG_MASK(ARG_FLAG_TIME), RETURN_TYPE_MASK(RETURN_TYPE_DOUBLE)},
    {bsStatic("file"), ARG_FLAG_MASK(ARG_FLAG_FILE), RETURN_TYPE_MASK(RETURN_TYPE_BSTRING)},
    {bsStatic("dir"), ARG_FLAG_MASK(ARG_FLAG_DIR), RETURN_TYPE_MASK(RETURN_TYPE_BSTRING)},
    {bsStatic("boolean"), 0, RETURN_TYPE_MASK(RETURN_TYPE_BOOL)},
    {bsStatic("integer"), 0, RETURN_TYPE_MASK(RETURN_TYPE_INT)},
    {bsStatic("uint64"), 0, RETURN_TYPE_MASK(RETURN_TYPE_UINT64)},
#ifdef WITH_BSTRING
    {bsStatic("string"), 0, RETURN_TYPE_MASK(RETURN_TYPE_BSTRING)},
#else
    {bsStatic("string"), 0, RETURN_TYPE_MASK(RETURN_TYPE_STRING)},
#endif
    {bsStatic("float"), 0, RETURN_TYPE_MASK(RETURN_TYPE_FLOAT)},
    {bsStatic("double"), 0, RETURN_TYPE_MASK(RETURN_TYPE_DOUBLE)},
    {bsStatic("EOF"), 0, 0},
};



void usage_print_header();
void print_usage();

void usage_print_basetestopts();
void usage_print_baseopts(int message);

void print_test_usage(TestConfig_t cfg);

int parse_baseopts(int argc, char* argv[], RuntimeConfig* config);
int parse_testopts(int argc, char* argv[], TestConfig* tcfg, RuntimeConfig* config);
int check_required_params(TestConfig* tcfg, RuntimeConfig* config);

#endif /* CLI_PARSER */
