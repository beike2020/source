/*****************************************************************************
 * Function:	A example of loop queue operation.
 * Author: 	forwarding2012@yahoo.com.cn			  		 
 * Date:		2012.01.01
 * Complie: 	gcc -Wall -g Queue_loop.c -o Queue_loop
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

#define  MAXQSIZE 	1024
typedef int ElemType;

typedef struct SLQNode {
	ElemType data[MAXQSIZE];
	int front;
	int rear;
} SLQueue;

int init_SLQ(SLQueue ** Q)
{
	if ((*Q = (SLQueue *) malloc(sizeof(SLQueue))) == NULL)
		return -1;

	(*Q)->front = (*Q)->rear = 0;

	return 0;
}

void cleanElem_SLQ(SLQueue * Q)
{
	Q->front = Q->rear = 0;
}

void destory_SLQ(SLQueue * Q)
{
	if (Q != NULL)
		free(Q);

	Q->front = Q->rear = 0;
}

void show_SLQ(SLQueue * Q)
{
	int index;

	index = Q->front;
	while (index != Q->rear) {
		printf(" %d", (Q->data[index]));
		index = (index + 1) % MAXQSIZE;
	}
}

int isEmpty_SLQ(SLQueue * Q)
{
	if (Q->front == Q->rear)
		return 0;
	else
		return -1;
}

int getLength_SLQ(SLQueue * Q)
{
	return (Q->rear - Q->front + MAXQSIZE) % MAXQSIZE;
}

int getHead_SLQ(SLQueue * Q, ElemType * data)
{
	if (Q->front == Q->rear)
		return -1;

	*data = Q->data[Q->front];

	return 0;
}

int inQueue_SLQ(SLQueue * Q, ElemType data)
{
	if ((Q->rear + 1) % MAXQSIZE == Q->front)
		return -1;

	Q->data[Q->rear] = data;
	Q->rear = (Q->rear + 1) % MAXQSIZE;

	return 0;
}

int outQueue_SLQ(SLQueue * Q, ElemType * data)
{
	if (Q->front == Q->rear)
		return -1;

	*data = Q->data[Q->front];
	Q->front = (Q->front + 1) % MAXQSIZE;

	return 0;
}

int main()
{
	int index;
	ElemType data;
	SLQueue *squeue = NULL;
	ElemType elem[10] = { 6, 23, 22, 9, 1, 67, 15, 20, 9, 3 };

	init_SLQ(&squeue);

	printf("\nEnQueue loopQueue:");
	for (index = 0; index < 10; index++)
		inQueue_SLQ(squeue, elem[index]);
	show_SLQ(squeue);

	if (isEmpty_SLQ(squeue) == 0)
		printf("\nStatus  loopQueue: empty!");
	else
		printf("\nStatus  loopQueue: isn't empty");

	if ((index = getLength_SLQ(squeue)) == 0)
		printf("\nLength  loopQueue: 0!");
	else
		printf("\nLength  loopQueue: %d", index);

	if (getHead_SLQ(squeue, &data) == -1)
		printf("\nHead    loopQueue: no!");
	else
		printf("\nHead    loopQueue: %d", data);

	printf("\nDeQueue loopQueue: ");
	while (squeue->front != squeue->rear) {
		outQueue_SLQ(squeue, &data);
		printf("%d ", data);
	}

	cleanElem_SLQ(squeue);
	destory_SLQ(squeue);
	printf("\nClear and destory finish, quit!\n");

	return 0;
}
