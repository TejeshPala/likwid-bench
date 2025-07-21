#ifndef CLI_PARSER_H
#define CLI_PARSER_H

#include "bstrlib.h"
#include "test_types.h"
#include "results.h"

typedef enum {
    no_argument = 0,
    required_argument,
    multi_argument,
    sticky_with_argument,
    sticky_no_argument,
} CliOptionArgSpecifier;

typedef struct {
    char*       name;
    char        symbol;
    int         has_arg;
    char*       description;
} ConstCliOption;

typedef struct {
    int num_options;
    ConstCliOption *options;
} ConstCliOptions;

typedef struct {
    bstring name;
    bstring symbol;
    int has_arg;
    bstring description;
    bstring value;
    struct bstrList *values;
} CliOption;


typedef struct cli_options {
    int num_options;
    CliOption *options;
    bstring title;
    bstring prolog;
    bstring epilog;
} CliOptions;


int addConstCliOptions(CliOptions* options, ConstCliOptions *constopts);
int addCliOption(CliOptions* options, CliOption* new);
int parseCliOptions(struct bstrList* argv, CliOptions* options);

void cliOptionsTitle(CliOptions* options, bstring title);
void cliOptionsProlog(CliOptions* options, bstring prolog);
void cliOptionsEpilog(CliOptions* options, bstring epilog);
void printCliOptions(CliOptions* options);

void destroyCliOptions(CliOptions* options);

static ConstCliOption _basecliopts[] = {
    {"help", 'h', no_argument, "Help text and usage"},
    {"all", 'a', no_argument, "List available benchmarks"},
    {"verbose", 'V', required_argument, "Verbosity level (0 - 3)"},
    {"test", 't', required_argument, "Test name"},
    {"file", 'f', required_argument, "Test file"},
    {"kfolder", 'K', required_argument, "Folder with test files to search for test name"},
    {"tmpfolder", 'D', required_argument, "Temporary folder for the object files"},
    {"iterations", 'i', required_argument, "Iterations"},
    {"compiler", 'C', required_argument, "Select compiler (gcc, icc, icx, clang)"},
    {"runtime", 'r', required_argument, "Possible Units: ms, s, m, h. Default: s. Runtime"},
    {"output", 'o', required_argument, "Set output: 'stdout', 'stderr' or a filename"},
    {"csv", 'O', no_argument, "Output results in CSV format"},
    {"json", 'J', no_argument, "Output results in JSON format"},
    {"detailed", 'd', no_argument, "Output detailed results (cycles and frequency will be printed)"},
    {"printdomains", 'p', no_argument, "List available domains available on the architecture"},
};

static ConstCliOptions basecliopts = {
    .num_options = 15,
    .options = _basecliopts,
};

static ConstCliOption _wgroupopts[] = {
    {"workgroup", 'w', multi_argument, "Workgroup definition"},
};

static ConstCliOptions wgroupopts = {
    .num_options = 1,
    .options = _wgroupopts,
};

static ConstCliOption _basetestopts[] = {
    {"iterations", 'i', required_argument, "Iterations"},
    {"runtime", 'r', required_argument, "Runtime"},
};

static ConstCliOptions basetestopts = {
    .num_options = 2,
    .options = _basetestopts,
};

int assignBaseCliOptions(CliOptions* options, RuntimeConfig* runcfg);
int generateTestCliOptions(CliOptions* options, RuntimeConfig* runcfg);
int assignTestCliOptions(CliOptions* options, RuntimeConfig* runcfg);
int assignWorkgroupCliOptions(CliOptions* options, RuntimeConfig* runcfg);

#endif
