/******************************************************************************
 * Function:	A example of order table operation.
 * Author:	forwarding2012@yahoo.com.cn				  
 * Date:		2012.01.01 					   
 * Complie: 	gcc -Wall -g Table_order.c -o Table_order
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>

#define  maxsize 50

typedef struct _sqlist {
	int list[maxsize];
	int key;
	int size;
	int data;
} SeqList;

void clean_OTable(SeqList * p)
{
	p->size = 0;
}

void show_OTable(SeqList * p)
{
	int j;

	if (p->size == 0) {
		printf("Order_Table is empty!\n");
	} else if (p->size == 1) {
		printf("%d", p->list[p->size]);
	} else {
		for (j = 0; j < p->size - 1; j++)
			printf("%d->", p->list[j]);
		printf("%d", p->list[j]);
	}
}

int length_OTable(SeqList * p)
{
	return p->size;
}

int getByIndex_OTable(SeqList * p, int index)
{
	if (index < 1 || index > p->size)
		return -1;

	return p->list[index - 1];
}

int getByKey_OTable(SeqList * p, int key)
{
	int index = 0;

	while (index < p->size && p->list[index] != key)
		index++;

	if (index == p->size)
		return -1;

	return index + 1;
}

int insertByIndex_OTable(SeqList * p, int index, int key)
{
	int j;

	if (index < 1 || index > p->size)
		return -1;

	for (j = p->size; j >= index; j--)
		p->list[j] = p->list[j - 1];

	p->list[j] = key;
	p->size++;

	return 0;
}

int deleteByIndex_OTable(SeqList * p, int index)
{
	int j;

	if (index < 1 || index > p->size)
		return -1;

	for (j = index - 1; j < p->size; j++)
		p->list[j] = p->list[j + 1];

	p->size--;

	return 0;
}

void sortByKey_OTable(SeqList * p, int n)
{
	int i, j, tmp;

	for (i = 0; i < n; i++) {
		for (j = i + 1; j < n; j++) {
			if (p->list[i] > p->list[j]) {
				tmp = p->list[i];
				p->list[i] = p->list[j];
				p->list[j] = tmp;
			}
		}
	}
}

int main()
{
	SeqList *slist;
	int index = 0, key = 0, len = 0;
	int elem[10] = { 16, 8, 23, 2, 31, 13, 19, 87, 7, 24 };

	if ((slist = (SeqList *) malloc(sizeof(SeqList))) == NULL)
		return -1;

	for (index = 0; index < 10; index++) {
		slist->list[index] = elem[index];
		slist->size++;
	}

	printf("Order_Table as follow: \n\t");
	show_OTable(slist);
	len = length_OTable(slist);
	printf(",len %d", len);

	printf("\nIndex-3 of Order_Table search:\n\t");
	key = getByIndex_OTable(slist, 3);
	printf("Key: %d", key);

	printf("\nKey-87 of Order_Table search:\n\t");
	index = getByKey_OTable(slist, 87);
	printf("Index: %d", index);

	printf("\nInsert [5,5] to Order_Table:\n\t");
	insertByIndex_OTable(slist, 5, 5);
	show_OTable(slist);

	printf("\nDelete index-5 of Order_Table delete:\n\t");
	deleteByIndex_OTable(slist, 5);
	show_OTable(slist);

	printf("\nAfter sort the Order_Table:\n\t");
	sortByKey_OTable(slist, 10);
	show_OTable(slist);

	clean_OTable(slist);
	free(slist);
	printf("\nClear and Destory finish, quit!\n");

	return 0;
}
