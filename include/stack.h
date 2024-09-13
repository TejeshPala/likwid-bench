typedef struct
{
	void **content;
	int size;
	int top;
} Stack;

#define FRESH_STACK {NULL, 0, -1}

int stackInit(Stack *s, int size);
int stackPush(Stack *s, void* val);
void* stackTop(Stack *s);
void* stackPop(Stack *s);
int stackSize(Stack *s);
void stackFree(Stack *s);
void stackPrint(Stack *s);
