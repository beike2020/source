/*****************************************************************************
 * Function:	A example of share stack operation.
 * Author: 	forwarding2012@yahoo.com.cn			  		 
 * Date:		2012.01.01			
 * Complie:	gcc -Wall -g Stack_share.c -o Stack_share
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>

#define MAXSIZE 20

typedef int ElemType;
typedef struct DSSNode {
	ElemType data[MAXSIZE];
	int top1;
	int top2;
} DSStack;

int init_DSS(DSStack * S)
{
	S->top1 = -1;
	S->top2 = MAXSIZE;

	return 0;
}

int clean_DSS(DSStack * S)
{
	S->top1 = -1;
	S->top2 = MAXSIZE;

	return 0;
}

void show_DSS(DSStack S)
{
	int i = 0;

	while (i <= S.top1)
		printf(" %d", S.data[i++]);

	i = S.top2;
	while (i < MAXSIZE)
		printf(" %d", S.data[i++]);
}

int isEmpty_DSS(DSStack S)
{
	if (S.top1 == -1 && S.top2 == MAXSIZE)
		return 0;
	else
		return -1;
}

int getLength_DSS(DSStack S)
{
	return (S.top1 + 1) + (MAXSIZE - S.top2);
}

int push_DSS(DSStack * S, ElemType data, int number)
{
	if (S->top1 + 1 == S->top2)
		return -1;

	if (number == 1)
		S->data[++S->top1] = data;
	else if (number == 2)
		S->data[--S->top2] = data;

	return 0;
}

int pop_DSS(DSStack * S, ElemType * data, int number)
{
	if (number == 1) {
		if (S->top1 == -1)
			return -1;
		*data = S->data[S->top1--];
	} else if (number == 2) {
		if (S->top2 == MAXSIZE)
			return -1;
		*data = S->data[S->top2++];
	}

	return 0;
}

int main()
{
	DSStack s;
	int i, data;

	if (init_DSS(&s) != 0)
		return -1;

	for (i = 1; i <= 5; i++)
		push_DSS(&s, i, 1);

	for (i = MAXSIZE; i >= MAXSIZE - 2; i--)
		push_DSS(&s, i, 2);

	printf("\nAfter init Stack S, then S:\n\t");
	show_DSS(s);

	pop_DSS(&s, &data, 2);
	printf("\nPop Stack top data:\n\t %d", data);
	printf("\nS %d[0-empty], len:\n\t %d", isEmpty_DSS(s),
	       getLength_DSS(s));

	for (i = 6; i <= MAXSIZE - 2; i++)
		push_DSS(&s, i, 1);
	printf("\nPush Stack S, then S:\n\t");
	show_DSS(s);

	printf("\nS full? %d[0-no]", push_DSS(&s, 100, 1));
	clean_DSS(&s);
	printf("\nClean Stack S, then S empty? %d[0-yes]\n", isEmpty_DSS(s));

	return 0;
}
