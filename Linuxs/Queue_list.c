/*****************************************************************************
 * Function:	A example of loop linklist queue operation.
 * Author: 	forwarding2012@yahoo.com.cn			  		 
 * Date:		2012.01.01		
 * Complie:	gcc -Wall -g Queue_list.c -o Queue_list
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

typedef int ElemType;

typedef struct QNode {
	ElemType data;
	struct QNode *next;
} QueueNode;

typedef struct lqnode {
	QueueNode *front;
	QueueNode *rear;
} LLinkQueue;

int init_LLQ(LLinkQueue ** Q)
{
	if ((*Q = (LLinkQueue *) malloc(sizeof(LLinkQueue))) == NULL)
		return -1;

	(*Q)->front = (*Q)->rear = (QueueNode *) malloc(sizeof(QueueNode));
	if ((*Q)->front == NULL)
		return -1;

	(*Q)->front->next = NULL;

	return 0;
}

int clean_LLQ(LLinkQueue * Q)
{
	QueueNode *P, *T;

	Q->rear = Q->front;
	P = Q->front->next;
	Q->front->next = NULL;
	while (P) {
		T = P;
		P = P->next;
		free(T);
	}

	return 0;
}

int destroy_LLQ(LLinkQueue * Q)
{
	while (Q->front) {
		Q->rear = Q->front->next;
		free(Q->front);
		Q->front = Q->rear;
	}

	free(Q);

	return 0;
}

void show_LLQ(LLinkQueue * Q)
{
	QueueNode *P;

	P = Q->front->next;
	while (P) {
		printf(" %d", P->data);
		P = P->next;
	}
}

int isEmpty_LLQ(LLinkQueue * Q)
{
	if (Q->front == Q->rear)
		return 0;
	else
		return -1;
}

int getLength_LLQ(LLinkQueue * Q)
{
	int index = 0;
	QueueNode *P;

	P = Q->front;
	while (Q->rear != P) {
		index++;
		P = P->next;
	}

	return index;
}

int getHead_LLQ(LLinkQueue * Q, ElemType * e)
{
	QueueNode *P;

	if (Q->front == Q->rear)
		return -1;

	P = Q->front->next;
	*e = P->data;

	return 0;
}

int inQueue_LLQ(LLinkQueue * Q, ElemType data)
{
	QueueNode *P;

	if ((P = (QueueNode *) malloc(sizeof(QueueNode))) == NULL)
		return -1;

	P->data = data;
	P->next = NULL;
	Q->rear->next = P;
	Q->rear = P;

	return 0;
}

int outQueue_LLQ(LLinkQueue * Q, ElemType * data)
{
	QueueNode *P;

	if (Q->front == Q->rear)
		return -1;

	P = Q->front->next;
	*data = P->data;
	Q->front->next = P->next;
	free(P);

	if (Q->front->next == NULL)
		Q->rear = Q->front;

	return 0;
}

int main()
{
	int i, ret;
	ElemType data;
	LLinkQueue *q = NULL;
	ElemType elem[10] = { 16, 8, 23, 2, 31, 13, 19, 87, 7, 24 };

	init_LLQ(&q);

	printf("\nEnQueue LinkQueue:");
	for (i = 0; i < 10; i++)
		inQueue_LLQ(q, elem[i]);
	show_LLQ(q);

	if (isEmpty_LLQ(q) == 0)
		printf("\nStatus  linkQueue: empty!");
	else
		printf("\nStatus  linkQueue: isn't empty");

	ret = getLength_LLQ(q);
	printf("\nLength  linkQueue: %d", ret);

	if ((ret = getHead_LLQ(q, &data)) == -1)
		printf("\nHead    linkQueue: no!");
	else
		printf("\nHead    linkQueue: %d", data);

	printf("\nDeQueue LinkQueue:");
	while (q->front != q->rear) {
		outQueue_LLQ(q, &data);
		printf(" %d", data);
	}

	clean_LLQ(q);
	destroy_LLQ(q);
	printf("\nClear and Destory finish, quit!\n");

	return 0;
}
