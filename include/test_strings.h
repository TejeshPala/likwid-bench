// test_strings.h
#ifndef TEST_STRINGS
#define TEST_STRINGS


#include "bstrlib.h"
#include "bstrlib_helper.h"

#ifdef __cplusplus
extern "C" {
#endif

static struct tagbstring biterations = bsStatic("ITER");
static struct tagbstring bnumthreads = bsStatic("NUM_THREADS");
static struct tagbstring bgroupid = bsStatic("GROUP_ID");
static struct tagbstring bthreadid = bsStatic("THREAD_ID");
static struct tagbstring bthreadcpu = bsStatic("THREAD_CPU"); 
static struct tagbstring bglobalid = bsStatic("GLOBAL_ID");

static struct tagbstring btrue = bsStatic("true");
static struct tagbstring bfalse = bsStatic("false");
static struct tagbstring bspace = bsStatic(" ");
static struct tagbstring bunderscroll = bsStatic("_");
static struct tagbstring bempty = bsStatic("");

static struct tagbstring bstats1[] =
{
    bsStatic("cycles"),
    bsStatic("freq"),
    bsStatic("iters"),
    bsStatic("time"),
};

static struct tagbstring bstats2[] =
{
    bsStatic("iters"),
    bsStatic("time"),
};

#ifdef __cplusplus
}
#endif


#endif /* TEST_STRINGS */
