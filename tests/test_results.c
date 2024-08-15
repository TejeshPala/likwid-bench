#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <results.h>
#include <test_types.h>
#include <helper.h>
#include <error.h>
#include <bstrlib.h>

int cpus[8] = {1,2,3,4,5,6,7,0};

int global_verbosity = DEBUGLEV_DEVELOP;

static struct tagbstring NUM_THREADS_KEY = bsStatic("NUM_THREADS");
static struct tagbstring NUM_THREADS_VAL = bsStatic("2");
static struct tagbstring TIME_KEY = bsStatic("TIME");
static double            TIME_VAL = 42.12345;
static struct tagbstring TEST_CALC = bsStatic("TIME/NUM_THREADS + NUM_THREADS*(TIME)");

int main(int argc, char* argv)
{
    int err = 0;
    RuntimeWorkgroupResult single;
    err = init_result(&single);
    if (err != 0)
    {
        printf("Error initializing single result\n");
        return -1;
    }
    err = add_variable(&single, &NUM_THREADS_KEY, &NUM_THREADS_VAL);
    if (err != 0)
    {
        printf("Error adding variable %s=%s to single result\n", bdata(&NUM_THREADS_KEY), bdata(&NUM_THREADS_VAL));
        return -1;
    }
    err = add_value(&single, &TIME_KEY, TIME_VAL);
    if (err != 0)
    {
        printf("Error adding value %s=%f to single result\n", bdata(&TIME_KEY), TIME_VAL);
        return -1;
    }
    bstring t = bstrcpy(&TEST_CALC);
    printf("Before %s\n", bdata(t));
    err = replace_all(&single, t, NULL);
    printf("After %s\n", bdata(t));
    bdestroy(t);
    destroy_result(&single);
    return 0;
}


