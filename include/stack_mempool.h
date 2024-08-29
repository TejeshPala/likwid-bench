#ifndef CALCULATOR_STACK_MEMPOOL_H
#define CALCULATOR_STACK_MEMPOOL_H

#include <mempool.h>

typedef struct
{
	void **content;
	int size;
	int top;
} Stack;

#define FRESH_STACK {NULL, 0, -1}

int stackInit(MemPool_t pool, Stack *s, int size);
int stackPush(Stack *s, void* val);
void* stackTop(Stack *s);
void* stackPop(Stack *s);
int stackSize(Stack *s);
void stackFree(MemPool_t pool, Stack *s);
void stackPrint(Stack *s);


#endif
