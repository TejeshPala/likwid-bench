#ifndef DYNLOAD_H
#define DYNLOAD_H

#include "bstrlib.h"

#define NUM_COMPILER_CANDIDATES 2
static struct tagbstring compiler_candidates[NUM_COMPILER_CANDIDATES] = {
    bsStatic("gcc"),
    bsStatic("icc"),
};



#endif /* DYNLOAD_H */
