#ifndef ERROR_H
#define ERROR_H

#include <errno.h>
#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define HLINE "-------------------------------------------------------------\n"
#define SLINE "*************************************************************\n"

#define DEBUGLEV_ONLY_ERROR 0
#define DEBUGLEV_INFO 1
#define DEBUGLEV_DETAIL 2
#define DEBUGLEV_DEVELOP 3


extern int global_verbosity;

#define ERROR_PRINT(msg, ...) \
    fprintf(stderr, "ERROR - [%s:%s:%d] %s: " msg "\n", __FILE__, __func__,__LINE__, strerror(errno), ##__VA_ARGS__);

#define WARN_PRINT(msg, ...) \
    fprintf(stderr, "WARN - " msg "\n", ##__VA_ARGS__);

#define DEBUG_PRINT(lev, fmt, ...) \
    if (((lev) >= 0) && ((lev) <= global_verbosity)) { \
        fprintf(stdout, "DEBUG - [%s:%d] " fmt "\n", __func__, __LINE__, ##__VA_ARGS__); \
        fflush(stdout); \
    }

#define INFO_PRINT(fmt, ...) \
    if (global_verbosity >= DEBUGLEV_INFO) \
    { \
        fprintf(stdout, "INFO - " fmt "\n", ##__VA_ARGS__); \
    }

#define TODO_PRINT(fmt, ...)  \
    fprintf(stdout, "TODO - " fmt "\n", ##__VA_ARGS__);


#endif /*ERROR_H*/
