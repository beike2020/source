/******************************************************************************
 * Function:	A example of seq stack operation.
 * Author: 	forwarding2012@yahoo.com.cn			  		 
 * Date:		2012.01.01				  		  
 * Complie:	gcc -Wall -g Stack_order.c -o Stack_order
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

#define  MAX 	1024
typedef int ElemType;
typedef struct seqstack {
	ElemType data[MAX];
	int top;
} SeqStack;

int init_SS(SeqStack ** S)
{
	if ((*S = (SeqStack *) malloc(sizeof(SeqStack))) == NULL)
		return -1;

	(*S)->top = -1;

	return 0;
}

void clean_SS(SeqStack * S)
{
	S->top = -1;
}

void destory_SS(SeqStack * S)
{
	free(S);
}

void show_SS(SeqStack * S)
{
	int index = 0;

	while (index <= S->top)
		printf(" %d", S->data[index++]);
}

int isEmpty_SS(SeqStack * S)
{
	if (S->top == -1)
		return 0;
	else
		return -1;
}

int getLength_SS(SeqStack * S)
{
	return S->top + 1;
}

int getTop_SS(SeqStack * S, ElemType * data)
{
	if (S->top == -1)
		return -1;

	*data = S->data[S->top];

	return 0;
}

int push_SS(SeqStack * S, ElemType data)
{
	if (S->top == MAX - 1)
		return -1;

	S->top++;
	S->data[S->top] = data;

	return 0;
}

int pop_SS(SeqStack * S, ElemType * data)
{
	if (S->top == -1)
		return -1;

	*data = S->data[S->top];
	S->top--;

	return 0;
}

int main()
{
	int index;
	ElemType data;
	SeqStack *stack;
	ElemType elem[10] = { 6, 23, 22, 9, 1, 67, 15, 20, 9, 3 };

	init_SS(&stack);

	printf("\nPushStack SeqStack:");
	for (index = 0; index < 10; index++)
		push_SS(stack, elem[index]);
	show_SS(stack);

	if (isEmpty_SS(stack) == 0)
		printf("\nStatus    SeqStack: empty!");
	else
		printf("\nStatus    SeqStack: isn't empty");

	if ((index = getLength_SS(stack)) == 0)
		printf("\nLength    SeqStack: 0!");
	else
		printf("\nLength    SeqStack: %d", index);

	if (getTop_SS(stack, &data) == -1)
		printf("\nTop       SeqStack: no!");
	else
		printf("\nTop       SeqStack: %d", data);

	printf("\nPopStack  SeqStack:");
	while (stack->top != -1) {
		pop_SS(stack, &data);
		printf(" %d", data);
	}

	clean_SS(stack);
	destory_SS(stack);
	printf("\nClear and destory finish, quit!\n");

	return 0;
}
