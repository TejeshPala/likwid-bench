#ifndef CLI_PARSER_H
#define CLI_PARSER_H

#include "bstrlib.h"
#include "test_types.h"

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
    {"verbose", 'V', required_argument, "Verbosity level (0 - 3)"},
    {"test", 't', required_argument, "Test name"},
    {"file", 'f', required_argument, "Test file"},
    {"kfolder", 'K', required_argument, "Folder with test files to search for test name"},
    {"tmpfolder", 'D', required_argument, "Temporary folder for the object files"},
    {"workgroup", 'w', multi_argument, "Workgroup definition"},
    {"arraysize", 'N', required_argument, "Size of array that should be loaded, Possible Values:kB, MB, GB"},
    {"iterations", 'i', required_argument, "Iterations"},
    {"runtime", 'r', required_argument, "Runtime"},
};

static ConstCliOptions basecliopts = {
    .num_options = 10,
    .options = _basecliopts,
};

static ConstCliOption _wgroupopts[] = {
    {"workgroup", 'w', multi_argument, "Workgroup definition"},
};

static ConstCliOptions wgroupopts = {
    .num_options = 1,
    .options = _wgroupopts,
};

static ConstCliOption _testopts[] = {
    {"iterations", 'i', required_argument, "Iterations"},
    {"runtime", 'r', required_argument, "Runtime"},
};

static ConstCliOptions testopts = {
    .num_options = 2,
    .options = _testopts,
};

int assignBaseCliOptions(CliOptions* options, RuntimeConfig* runcfg);
int generateTestCliOptions(CliOptions* options, RuntimeConfig* runcfg);
int assignTestCliOptions(CliOptions* options, RuntimeConfig* runcfg);

#endif
