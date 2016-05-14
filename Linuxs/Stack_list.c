/*****************************************************************************
 * Function:	A example of linklist stack operation.
 * Author: 	forwarding2012@yahoo.com.cn			  		 
 * Date:		2012.01.01
 * Complie:	gcc -Wall -g Stack_list.c -o Stack_list
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

typedef int ElemType;

typedef struct LSNode {
	ElemType data;
	struct LSNode *next;
} LLinkStack;

int init_LLS(LLinkStack ** top)
{
	if ((*top = (LLinkStack *) malloc(sizeof(LLinkStack))) == NULL)
		return -1;

	(*top)->data = 0;
	(*top)->next = NULL;

	return 0;
}

void clean_LLS(LLinkStack * top)
{
	LLinkStack *p = NULL;
	LLinkStack *tmp = top->next;

	while (tmp && tmp->next) {
		p = tmp->next;
		tmp->next = tmp->next->next;
		free(p);
	}
}

void destroy_LLS(LLinkStack * top)
{
	LLinkStack *p = NULL;
	LLinkStack *tmp = top;

	while (tmp) {
		p = tmp->next;
		free(tmp);
		tmp = p;
	}
}

int isEmpty_LLS(LLinkStack * top)
{
	LLinkStack *p = top;

	if (p->next)
		return 0;
	else
		return 1;
}

int getLength_LLS(LLinkStack * top)
{
	int length = 0;
	LLinkStack *p = top->next;

	while (p) {
		p = p->next;
		++length;
	}

	return length;
}

int getHead_LLS(LLinkStack * top)
{
	LLinkStack *tmp;

	tmp = top->next;
	if (tmp == NULL)
		return -1;

	return tmp->data;
}

void pushStack_LLS(LLinkStack * top, ElemType data)
{
	LLinkStack *p;

	if ((p = (LLinkStack *) malloc(sizeof(LLinkStack))) == NULL)
		return;

	p->data = data;
	p->next = top->next;
	top->next = p;
}

int popStack_LLS(LLinkStack * top)
{
	LLinkStack *tmp = top->next;

	while (tmp) {
		printf(" %d", tmp->data);
		tmp = tmp->next;
	}

	return 0;
}

int main()
{
	int i, len;
	ElemType data;
	LLinkStack *lstack;
	ElemType elem[10] = { 16, 8, 23, 2, 31, 13, 19, 87, 7, 24 };

	init_LLS(&lstack);

	printf("\nPush linkStack: ");
	for (i = 0; i < 10; i++)
		pushStack_LLS(lstack, elem[i]);

	if (isEmpty_LLS(lstack) == 1)
		printf("\nStatus linkStack: empty!");
	else
		printf("\nStatus linkStack: isn't empty");

	if ((len = getLength_LLS(lstack)) == 0)
		printf("\nLength LLinkStack *: empty!");
	else
		printf("\nLength linkStack: %d", len);

	if ((data = getHead_LLS(lstack)) == -1)
		printf("\nHead   linkStack: no!");
	else
		printf("\nHead   linkStack: %d", data);

	printf("\nPop    linkStack:");
	popStack_LLS(lstack);

	clean_LLS(lstack);
	destroy_LLS(lstack);
	printf("\nClear and destory finish, quit!\n");

	return 0;
}
