#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "stack.h"

int stackInit(Stack *s, int size)
{
	if ((!s) || (size <= 0))
	{
		return -EINVAL;
	}
	s->content = malloc(size * sizeof(void*));
	if (!s->content)
	{
		return -ENOMEM;
	}
	s->size = size;
	s->top = -1;
	return 0;
}

int stackPush(Stack *s, void* val)
{
	/*if(s->top + 1 >= s->size) // If stack is full
	{
		(s->size)++;
		s->content = (void**)realloc(s->content, s->size * sizeof(void*));
	}*/
	if ((!s) || (!val))
	{
		return -EINVAL;
	}
	if (s->top + 1 >= s->size)
	{
		return -ENOBUFS;
	}

	(s->top)++;
	s->content[s->top] = val;
	return 0;
}

void* stackTop(Stack *s)
{
	void *ret = NULL;
	if(s && s->top >= 0 && s->content != NULL)
		ret = s->content[s->top];
	return ret;
}

void* stackPop(Stack *s)
{
	void *ret = NULL;
	if(s && s->top >= 0 && s->content != NULL)
		ret = s->content[(s->top)--];
	return ret;
}

int stackSize(Stack *s)
{
	int size = 0;
	if (s) { size = s->top + 1; }
	return size;
}

void stackFree(Stack *s)
{
	if (!s) return;
	if (s->content)
		free(s->content);
	s->content = NULL;
	s->size = 0;
	s->top = -1;
}
