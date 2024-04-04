#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "stack.h"

#define STACKSIZE 100

// Testing stackFree
int testStackFreeInvalid()
{
    Stack strs = FRESH_STACK;
    stackFree(&strs);
    return (strs.content == NULL);
}

int testStackFreeInvalid2()
{
    Stack strs;
    stackFree(&strs);
    return (strs.content == NULL);
}

// Testing stackInit
int testStackInitNegative()
{
    Stack strs = FRESH_STACK;
    int err = stackInit(&strs, -1);
    if (strs.content != NULL)
    {
        stackFree(&strs);
    }
    return (err == -EINVAL);
}

int testStackInitZero()
{
    Stack strs = FRESH_STACK;
    int err = stackInit(&strs, 0);
    if (strs.content != NULL)
    {
        stackFree(&strs);
    }
    return (err == -EINVAL);
}

int testStackInitValid()
{
    Stack strs = FRESH_STACK;
    int err = stackInit(&strs, STACKSIZE);
    if (strs.content != NULL)
    {
        stackFree(&strs);
    }
    return (err == 0);
}


// Testing stackSize
int testStackSizeOnlyInit()
{
    Stack strs = FRESH_STACK;
    return stackSize(&strs) == 0;
}

int testStackSizeFresh()
{
    Stack strs = FRESH_STACK;
    return stackSize(&strs) == 0;
}

int testStackSizeInit()
{
    Stack strs = FRESH_STACK;
    int err = stackInit(&strs, STACKSIZE);
    if (err != 0) {
        return 0;
    }
    err = (stackSize(&strs) == 0);
    stackFree(&strs);
    return err;
}

int testStackSizeFreed()
{
    Stack strs = FRESH_STACK;
    int err = stackInit(&strs, STACKSIZE);
    if (err != 0) {
        return 0;
    }
    stackFree(&strs);
    err = (stackSize(&strs) == 0);
    return err;
}

int testStackSizeTwice()
{
    char* str = "foo";
    Stack strs = FRESH_STACK;
    int err = stackInit(&strs, STACKSIZE);
    if (err != 0) {
        return 0;
    }
    err = stackPush(&strs, str);
    if (err != 0) {
        return 0;
    }
    err = stackPush(&strs, str);
    if (err != 0) {
        return 0;
    }
    err = (stackSize(&strs) == 2);
    stackFree(&strs);
    return err;
}

// Testing stackTop
int testStackTopFresh()
{
    Stack strs = FRESH_STACK;
    return (stackTop(&strs) == NULL);
}

int testStackTopInit()
{
    Stack strs = FRESH_STACK;
    int err = stackInit(&strs, STACKSIZE);
    if (err != 0) {
        return 0;
    }
    err = (stackTop(&strs) == NULL);
    stackFree(&strs);
    return err;
}

// Testing stackPush
int testStackPushNull()
{
    Stack strs = FRESH_STACK;
    int err = stackInit(&strs, STACKSIZE);
    if (err != 0) {
        return 0;
    }
    err = stackPush(&strs, NULL);
    stackFree(&strs);
    return (err == -EINVAL);
}

int testStackPushValid()
{
    char* str = "foo";
    Stack strs = FRESH_STACK;
    int err = stackInit(&strs, STACKSIZE);
    if (err != 0) {
        return 0;
    }
    err = stackPush(&strs, str);
    stackFree(&strs);
    return (err == 0);
}

int testStackPushValidTwice()
{
    char* str = "foo";
    Stack strs = FRESH_STACK;
    int err = stackInit(&strs, STACKSIZE);
    if (err != 0) {
        return 0;
    }
    err = stackPush(&strs, str);
    if (err != 0) {
        return 0;
    }
    err = stackPush(&strs, str);
    stackFree(&strs);
    return (err == 0);
}

int testStackPushTooSmall()
{
    char* str = "foo";
    Stack strs = FRESH_STACK;
    int err = stackInit(&strs, 1);
    if (err != 0) {
        return 0;
    }
    err = stackPush(&strs, str);
    if (err != 0) {
        return 0;
    }
    err = stackPush(&strs, str);
    stackFree(&strs);
    return (err == -ENOBUFS);
}

// Testing stackPop
int testStackPopFresh()
{
    Stack strs = FRESH_STACK;
    return (stackPop(&strs) == NULL);
}

int testStackPopEmpty()
{
    Stack strs = FRESH_STACK;
    int err = stackInit(&strs, STACKSIZE);
    if (err != 0) {
        return 0;
    }
    char* data = stackPop(&strs);
    stackFree(&strs);
    return (data == NULL);
}

int testStackPopValid()
{
    char* str = "foo";
    Stack strs = FRESH_STACK;
    int err = stackInit(&strs, STACKSIZE);
    if (err != 0) {
        return 0;
    }
    err = stackPush(&strs, str);
    if (err != 0) {
        return 0;
    }
    char* new = stackPop(&strs);
    stackFree(&strs);
    return (new == str);
}


typedef struct {
    int (*stest_function)();
    int retvalue;
} StackTest;

static StackTest stack_tests[] = {
    {testStackFreeInvalid, 1},
    {testStackFreeInvalid2, 1},
    {testStackInitNegative, 1},
    {testStackInitZero, 1},
    {testStackInitValid, 1},
    {testStackSizeOnlyInit, 1},
    {testStackSizeInit, 1},
    {testStackSizeFreed, 1},
    {testStackPushNull, 1},
    {testStackTopFresh, 1},
    {testStackTopInit, 1},
    {testStackPushValid, 1},
    {testStackPushValidTwice, 1},
    {testStackPopFresh, 1},
    {testStackPopEmpty, 1},
    {testStackPopValid, 1},
    {testStackSizeTwice, 1},
    {testStackPushTooSmall, 1},
    {NULL, -1}
};


int main()
{
    int i = 0;
    int ok = 0;
    int err = 0;
    printf("Testing stack implementation\n");
    while (stack_tests[i].stest_function != NULL)
    {
        int ret = stack_tests[i].stest_function();
        if (ret == stack_tests[i].retvalue)
        {
            ok++;
        }
        else
        {
            err++;
        }
        i++;
    }
    printf("OK %d Error %d\n", ok, err);
    return 0;
}
