#ifndef GETOPT_EXTRA_H
#define GETOPT_EXTRA_H

#include <stdint.h>
#include <bstrlib.h>
#include <map.h>

typedef enum {
    ARG_FLAG_UNDEF = 0,
    ARG_FLAG_TIME,       
    ARG_FLAG_BYTES,      // size_t but accepts strings like '100kB', '1MB'. Output always in bytes.
    // Defines checks performed on the input
    ARG_FLAG_FILE,       // check that the file exists
    ARG_FLAG_DIR,        // check that the string is an existing directory
    ARG_FLAG_MAX
} CliOptionsArgFlag;
#define ARG_FLAG_MIN ARG_FLAG_UNDEF

#define ARG_FLAG_MASK(flag) (1ULL<<(flag))


typedef enum {
    // Defines the output data type
    RETURN_TYPE_BOOL = 0,   // unsigned
    RETURN_TYPE_INT,        // int
    RETURN_TYPE_UINT64,     // uint64_t
    RETURN_TYPE_BSTRING,    // bstring, see https://github.com/websnarf/bstrlib
    RETURN_TYPE_STRING,     // char* allocated
    RETURN_TYPE_FLOAT,      // float
    RETURN_TYPE_DOUBLE,     // double
    // Defines single value or list options
    RETURN_TYPE_LIST,       // all types but array of it, like int*, double* and bstring*. Length in qty member.
    RETURN_TYPE_MAX
} CliOptionsReturnType;
#define RETURN_TYPE_MIN RETURN_TYPE_BOOL

#define RETURN_TYPE_MASK(type) (1ULL<<(type))

// #define RETURN_TYPE_INT_FLAG (1<<RETURN_TYPE_INT)
// #define RETURN_TYPE_DOUBLE_FLAG (1<<RETURN_TYPE_DOUBLE)
// #define RETURN_TYPE_STRING_FLAG (1<<RETURN_TYPE_STRING)
// #define RETURN_TYPE_BOOL_FLAG (1<<RETURN_TYPE_BOOL)
// #define RETURN_TYPE_BSTRING_FLAG (1<<RETURN_TYPE_BSTRING)
// #define RETURN_TYPE_FLOAT_FLAG (1<<RETURN_TYPE_FLOAT)
// #define RETURN_TYPE_BYTES_FLAG (1<<RETURN_TYPE_BYTES)
// #define RETURN_TYPE_TIME_FLAG (1<<RETURN_TYPE_TIME)
// #define RETURN_TYPE_FILE_FLAG (1<<RETURN_TYPE_FILE)
// #define RETURN_TYPE_DIR_FLAG (1<<RETURN_TYPE_DIR)
// #define RETURN_TYPE_LIST_FLAG (1<<RETURN_TYPE_LIST)
// #define RETURN_TYPE_UINT64_FLAG (1<<RETURN_TYPE_UINT64)

#ifndef no_argument
#define	no_argument 0
#endif
#ifndef required_argument
#define required_argument 1
#endif
#ifndef optional_argument
#define optional_argument 2
#endif

typedef struct option_extra_return {
    int return_flag;
    int arg_flag;
    int qty;
    union {
        int intvalue;
        uint64_t uint64value;
        unsigned boolvalue;
        bstring bstrvalue;
        char* strvalue;
        float floatvalue;
        double doublevalue;
        int* intlist;
        struct bstrList* bstrlist;
        char** stringlist;
        float* floatlist;
        double* doublelist;
    } value;
} getopt_extra_return_t;


typedef struct option_extra {
    char* longname;
    char shortname;
    int return_flag;
    int has_arg;
    char* description;
    int arg_flag;
    int *flag;
    int val;
} getopt_extra_option_t;

int getopt_long_extra(int argc, char* argv[], char* shortopts, struct option_extra* options, Map_t *optionmap);


int add_option(struct option_extra* option, struct option_extra** options);
int add_option_params(char* longname, int shortname, int return_flag, int has_arg, struct option_extra** options, int arg_flag);
int add_option_list(struct option_extra* list, struct option_extra** out);
int del_option(char* longname, struct option_extra** options);
void print_options(struct option_extra* options);
void print_help(struct option_extra* options);

#endif
