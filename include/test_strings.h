// test_strings.h
#ifndef TEST_STRINGS
#define TEST_STRINGS


#include "bstrlib.h"
#include "bstrlib_helper.h"

#ifdef __cplusplus
extern "C" {
#endif

static struct tagbstring bstrptr = bsStatic("#STREAMPTRFORREPLACMENT");

static struct tagbstring biterations = bsStatic("ITER");
static struct tagbstring bnumthreads = bsStatic("NUM_THREADS");
static struct tagbstring bgroupid = bsStatic("GROUP_ID");
static struct tagbstring bthreadid = bsStatic("THREAD_ID");
static struct tagbstring bthreadcpu = bsStatic("THREAD_CPU"); 
static struct tagbstring bglobalid = bsStatic("GLOBAL_ID");
static struct tagbstring bbytesperiter = bsStatic("BYTES_PER_ITER");

static struct tagbstring btrue = bsStatic("true");
static struct tagbstring bfalse = bsStatic("false");
static struct tagbstring bspace = bsStatic(" ");
static struct tagbstring bunderscroll = bsStatic("_");
static struct tagbstring bempty = bsStatic("");

/*
 * bstats1 and bstats2 are used in `src/workgroups.c` -> `update_results` function
 * the stats should be alphabetical sorted and
 * needed to be updated in case new stats to be added in `update_results` for `values` list
 */
static struct tagbstring bstats1[] =
{
    bsStatic("cycles"),
    bsStatic("freq [Hz]"),
//    bsStatic("iters"),
    bsStatic("time"),
};

static int stats1_count = 3;

static struct tagbstring bstats2[] =
{
//    bsStatic("iters"),
    bsStatic("time"),
};

static int stats2_count = 1;

#ifdef __cplusplus
}
#endif


#endif /* TEST_STRINGS */
