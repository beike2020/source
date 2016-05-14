/*****************************************************************************
 * Function:	A example of static linklist operation.
 * Author: 	forwarding2012@yahoo.com.cn			  		 
 * Date:		2012.01.01			
 * Complie:	gcc -Wall -g List_static.c -o List_static
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>

#define MAXSIZE 1000

typedef int ElemType;

typedef struct SLNode {
	ElemType data;
	int cur;
} SLinkList[MAXSIZE];

int init_SLL(SLinkList space)
{
	int i;

	for (i = 0; i < MAXSIZE - 1; i++)
		space[i].cur = i + 1;
	space[MAXSIZE - 1].cur = 0;

	return 0;
}

int assign_SLL(SLinkList space)
{
	int i = space[0].cur;

	if (space[0].cur)
		space[0].cur = space[i].cur;

	return i;
}

void clean_SLL(SLinkList space, int k)
{
	space[k].cur = space[0].cur;
	space[0].cur = k;
}

int getLength_SLL(SLinkList L)
{
	int i, j = 0;

	i = L[MAXSIZE - 1].cur;
	while (i) {
		i = L[i].cur;
		j++;
	}

	return j;
}

int insert_SLL(SLinkList L, int i, ElemType e)
{
	int j, k, l;

	k = MAXSIZE - 1;
	if (i < 1 || i > getLength_SLL(L) + 1)
		return -1;

	j = assign_SLL(L);
	if (j == 0)
		return -1;

	L[j].data = e;
	for (l = 1; l <= i - 1; l++)
		k = L[k].cur;

	L[j].cur = L[k].cur;
	L[k].cur = j;

	return 0;
}

int delete_SLL(SLinkList L, int i)
{
	int j, k;

	if (i < 1 || i > getLength_SLL(L))
		return -1;

	k = MAXSIZE - 1;
	for (j = 1; j <= i - 1; j++)
		k = L[k].cur;

	j = L[k].cur;
	L[k].cur = L[j].cur;
	clean_SLL(L, j);

	return 0;
}

int traverse_SLL(SLinkList L)
{
	int i, j = 0;

	i = L[MAXSIZE - 1].cur;
	while (i) {
		printf(" %c", L[i].data);
		i = L[i].cur;
		j++;
	}

	return 0;
}

int main()
{
	SLinkList L;
	int i;

	i = init_SLL(L);
	printf("\nAfter init L, then L.length:\n\t %d", getLength_SLL(L));

	i = insert_SLL(L, 1, 'F');
	i = insert_SLL(L, 1, 'E');
	i = insert_SLL(L, 1, 'D');
	i = insert_SLL(L, 1, 'B');
	i = insert_SLL(L, 1, 'A');
	printf("\nAfter insert FEDBA to L, then L.data:\n\t");
	traverse_SLL(L);

	i = insert_SLL(L, 3, 'C');
	printf("\nAfter insert C between 'B' and 'D', then L.data:\n\t");
	traverse_SLL(L);

	i = delete_SLL(L, 1);
	printf("\nAfter delete A from L, then L.data:\n\t");
	traverse_SLL(L);
	printf("\n");

	return 0;
}
