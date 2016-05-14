/*****************************************************************************
 * Function:	A example of loop single linklist operation.
 * Author: 	forwarding2012@yahoo.com.cn				  		 	  	
 * Date:		2012.01.01
 * Complie: 	gcc -Wall -g List_oneLoop.c -o List_oneLoop
*****************************************************************************/
#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>

typedef int ElemType;

typedef struct SLLNode {
	ElemType data;
	struct SLLNode *next;
} SLLinkList;

int init_SLL(SLLinkList ** hNode)
{
	if ((*hNode = (SLLinkList *) malloc(sizeof(SLLinkList))) == NULL)
		return -1;

	(*hNode)->next = *hNode;

	return 0;
}

void clean_SLL(SLLinkList ** hNode)
{
	SLLinkList *pHold, *pNode;

	(*hNode) = (*hNode)->next;
	pNode = (*hNode)->next;
	while (pNode != *hNode) {
		pHold = pNode->next;
		free(pNode);
		pNode = pHold;
	}

	(*hNode)->next = (*hNode);
}

void destroy_SLL(SLLinkList ** hNode)
{
	SLLinkList *pHold, *pNode;

	pNode = (*hNode)->next;
	while (pNode != *hNode) {
		pHold = pNode->next;
		free(pNode);
		pNode = pHold;
	}

	free(*hNode);
	*hNode = NULL;
}

void show_SLL(SLLinkList * hNode)
{
	SLLinkList *pNode;

	pNode = hNode->next;
	while (pNode != hNode) {
		printf("%d ", pNode->data);
		pNode = pNode->next;
	}
}

int isEmpty_SLL(SLLinkList * hNode)
{
	if (hNode->next == hNode)
		return 0;
	else
		return -1;
}

int getLength_SLL(SLLinkList * hNode)
{
	int index = 0;
	SLLinkList *pNode;

	pNode = hNode->next;
	while (pNode != hNode) {
		++index;
		pNode = pNode->next;
	}

	return index;
}

int get_SLL_byIndex(SLLinkList * hNode, int dIndex, ElemType * data)
{
	int index = 1;
	SLLinkList *pNode;

	if (dIndex <= 0 || dIndex > getLength_SLL(hNode))
		return -1;

	pNode = hNode->next;
	while (index < dIndex) {
		++index;
		pNode = pNode->next;
	}

	*data = pNode->data;

	return 0;
}

int location_SLL_byData(SLLinkList * hNode, ElemType data)
{
	int index = 0;
	SLLinkList *pNode;

	pNode = hNode->next;
	while (pNode != hNode) {
		++index;
		if (pNode->data == data)
			return index;
		pNode = pNode->next;
	}

	return 0;
}

SLLinkList *getPrev_SLL_byData(SLLinkList * hNode, ElemType data)
{
	SLLinkList *pHold, *pNode;

	pNode = hNode->next->next;
	pHold = pNode->next;
	while (pHold != hNode->next) {
		if (pHold->data == data)
			return pNode;
		pNode = pHold;
		pHold = pHold->next;
	}

	return NULL;
}

SLLinkList *getNext_SLL_byData(SLLinkList * hNode, ElemType data)
{
	SLLinkList *pNode;

	pNode = hNode->next->next;
	while (pNode != hNode) {
		if (pNode->data == data)
			return pNode->next;
		pNode = pNode->next;
	}

	return NULL;
}

int insert_SLL_byIndex(SLLinkList ** hNode, ElemType data, int dIndex)
{
	int index = 0;
	SLLinkList *pHold, *pNode;

	if (dIndex <= 0 || dIndex > getLength_SLL(*hNode) + 1)
		return -1;

	pNode = *hNode;
	while (index < dIndex - 1) {
		pNode = pNode->next;
		++index;
	}

	if ((pHold = (SLLinkList *) malloc(sizeof(SLLinkList))) == NULL)
		return -1;

	pHold->data = data;
	pHold->next = pNode->next;
	pNode->next = pHold;

	if (pHold == *hNode)
		*hNode = pHold;

	return 0;
}

int delete_SLL_byIndex(SLLinkList ** hNode, int dIndex)
{
	int index = 0;
	SLLinkList *pHold, *pNode;

	if (dIndex <= 0 || dIndex > getLength_SLL(*hNode))
		return -1;

	pNode = *hNode;
	while (index < dIndex - 1) {
		pNode = pNode->next;
		++index;
	}

	pHold = pNode->next;
	pNode->next = pHold->next;
	if (*hNode == pHold)
		*hNode = pNode;
	free(pHold);

	return 0;
}

void sort_SLL_byData(SLLinkList * hNode)
{
	ElemType data;
	SLLinkList *pNode, *pHold;

	for (pNode = hNode->next; pNode && pNode != hNode; pNode = pNode->next) {
		for (pHold = pNode->next; pHold && pHold != hNode;
		     pHold = pHold->next) {
			if (pHold->data < pNode->data) {
				data = pHold->data;
				pHold->data = pNode->data;
				pNode->data = data;
			}
		}
	}
}

void merge_SLL_byIndex(SLLinkList * LA, SLLinkList * LB)
{
	SLLinkList *pNode, *pHold;

	pNode = LA;
	pHold = LB;
	while (pNode->next != LA)
		pNode = pNode->next;

	while (pHold->next != LB)
		pHold = pHold->next;

	pNode->next = LB->next;
	pHold->next = LA;
	free(LB);
}

int main()
{
	int i, index;
	ElemType item;
	SLLinkList *ListA, *ListB;
	ElemType elema[10] = { 16, 8, 23, 2, 31, 13, 19, 87, 7, 24 };
	ElemType elemb[10] = { 6, 23, 22, 9, 1, 67, 15, 20, 9, 3 };

	init_SLL(&ListA);
	init_SLL(&ListB);
	for (i = 0; i < 10; i++) {
		insert_SLL_byIndex(&ListA, elema[i], i + 1);
		insert_SLL_byIndex(&ListB, elemb[i], i + 1);
	}
	show_SLL(ListA);
	printf("\n");
	show_SLL(ListB);

	printf("\nInsert data-20 to index-5 in linkList_A: \n\t");
	insert_SLL_byIndex(&ListA, 20, 5);
	show_SLL(ListA);

	index = location_SLL_byData(ListA, 13);
	printf("\nSearch key-13 in linkList_A: \n\tindex-%d", index);

	get_SLL_byIndex(ListA, 5, &item);
	printf("\nSearch index-5 in linkList_A: \n\tdata-%d", item);

	printf("\nDelete index-3 in linkList_B: \n\t");
	delete_SLL_byIndex(&ListB, 3);
	show_SLL(ListB);

	printf("\nAfter sort the linklist: \n\t");
	sort_SLL_byData(ListA);
	sort_SLL_byData(ListB);
	show_SLL(ListA);
	printf("\n\t");
	show_SLL(ListB);

	printf("\nAfter merger two linklists: \n\t");
	merge_SLL_byIndex(ListA, ListB);
	show_SLL(ListA);

	clean_SLL(&ListA);
	destroy_SLL(&ListA);
	printf("\nClear and destory finish, quit!\n");

	return 0;
}
