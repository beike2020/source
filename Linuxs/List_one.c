/*****************************************************************************
 * Function:	A example of single linklist operation.
 * Author: 	forwarding2012@yahoo.com.cn			  		 
 * Date:		2012.01.01			
 * Complie:	gcc -Wall -g List_one.c -o List_one
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

typedef int ElemType;

typedef struct SNLNode {
	ElemType data;
	struct SNLNode *next;
} SNLinkList;

int init_SNL(SNLinkList ** hNode)
{
	if ((*hNode = (SNLinkList *) malloc(sizeof(SNLinkList))) == NULL)
		return -1;

	(*hNode)->next = NULL;

	return 0;
}

void clean_SNL(SNLinkList * hNode)
{
	SNLinkList *pHold, *pNode;

	pNode = hNode->next;
	while (pNode) {
		pHold = pNode->next;
		free(pNode);
		pNode = pHold;
	}
	hNode->next = NULL;
}

void destroy_SNL(SNLinkList ** hNode)
{
	SNLinkList *pNode;

	while (*hNode) {
		pNode = (*hNode)->next;
		free(*hNode);
		*hNode = pNode;
	}
}

void show_SNL(SNLinkList * hNode)
{
	SNLinkList *pNode;

	pNode = hNode->next;
	while (pNode) {
		printf("%d ", pNode->data);
		pNode = pNode->next;
	}
}

int isEmpty_SNL(SNLinkList * hNode)
{
	if (hNode->next == NULL)
		return 0;
	else
		return -1;
}

int getLength_SNL(SNLinkList * hNode)
{
	int index = 0;
	SNLinkList *pNode;

	pNode = hNode->next;
	while (pNode) {
		++index;
		pNode = pNode->next;
	}

	return index;
}

int get_SNL_byIndex(SNLinkList * hNode, int dIndex, ElemType * data)
{
	int index = 1;
	SNLinkList *pNode;

	if (dIndex <= 0 || dIndex > getLength_SNL(hNode))
		return -1;

	pNode = hNode->next;
	while (index < dIndex) {
		++index;
		pNode = pNode->next;
	}

	*data = pNode->data;

	return 0;
}

int location_SNL_byData(SNLinkList * hNode, ElemType data)
{
	int index = 0;
	SNLinkList *pNode;

	pNode = hNode->next;
	while (pNode) {
		++index;
		if (pNode->data == data)
			return index;
		pNode = pNode->next;
	}

	return 0;
}

SNLinkList *getPrev_SNL_byData(SNLinkList * hNode, ElemType data)
{
	SNLinkList *pHold, *pNode;

	pNode = hNode->next;
	while (pNode->next) {
		pHold = pNode->next;
		if (pHold->data == data)
			return pNode;
		pNode = pHold;
	}

	return NULL;
}

SNLinkList *getNext_SNL_byData(SNLinkList * hNode, ElemType data)
{
	SNLinkList *pNode;

	pNode = hNode->next;
	while (pNode->next) {
		if (pNode->data == data)
			return pNode->next;
		pNode = pNode->next;
	}

	return NULL;
}

int insert_SNL_byIndex(SNLinkList * hNode, ElemType data, int dIndex)
{
	int index = 0;
	SNLinkList *pHold, *pNode;

	if (dIndex <= 0 || dIndex > getLength_SNL(hNode) + 1)
		return -1;

	pNode = hNode;
	while (index < dIndex - 1) {
		pNode = pNode->next;
		++index;
	}

	if ((pHold = (SNLinkList *) malloc(sizeof(SNLinkList))) == NULL)
		return -1;

	pHold->data = data;
	pHold->next = pNode->next;
	pNode->next = pHold;

	return 0;
}

int delete_SNL_byIndex(SNLinkList * hNode, int dIndex)
{
	int index = 0;
	SNLinkList *pHold, *pNode;

	if (dIndex <= 0 || dIndex > getLength_SNL(hNode))
		return -1;

	pNode = hNode;
	while (pNode->next && index < dIndex - 1) {
		pNode = pNode->next;
		++index;
	}

	pHold = pNode->next;
	pNode->next = pHold->next;
	free(pHold);

	return 0;
}

void sort_SNL_byData(SNLinkList * hNode)
{
	ElemType data;
	SNLinkList *pNode, *pHold;

	for (pNode = hNode->next; pNode; pNode = pNode->next) {
		for (pHold = pNode->next; pHold; pHold = pHold->next) {
			if (pHold->data < pNode->data) {
				data = pHold->data;
				pHold->data = pNode->data;
				pNode->data = data;
			}
		}
	}
}

SNLinkList *merge_SNL_byData(SNLinkList * LA, SNLinkList * LB)
{
	SNLinkList *pmerg = NULL;

	if (LA == NULL)
		return LB;

	if (LB == NULL)
		return LA;

	if (LA->data < LB->data) {
		pmerg = LA;
		pmerg->next = merge_SNL_byData(LA->next, LB);
	} else {
		pmerg = LB;
		pmerg->next = merge_SNL_byData(LA, LB->next);
	}

	return pmerg;
}

void reverse_SNL_elem(SNLinkList * pNode)
{
	ElemType data;
	SNLinkList *p1, *p2, *p3;

	p1 = pNode->next;
	p2 = p1->next;
	p3 = p2->next;

	if (!p1->next)
		return;

	if (!p2->next) {
		data = p1->data;
		p1->data = p2->data;
		p2->data = data;
		return;
	}

	p1->next = NULL;
	while (p3->next) {
		p2->next = p1;
		p1 = p2;
		p2 = p3;
		p3 = p3->next;
	}

	p2->next = p1;
	p3->next = p2;
	pNode->next = p3;
}

int main()
{
	int i, index;
	ElemType item;
	SNLinkList *ListA, *ListB;
	ElemType elema[10] = { 16, 8, 23, 2, 31, 13, 19, 87, 7, 24 };
	ElemType elemb[10] = { 6, 23, 22, 9, 1, 67, 15, 20, 9, 3 };

	init_SNL(&ListA);
	init_SNL(&ListB);
	for (i = 0; i < 10; i++) {
		insert_SNL_byIndex(ListA, elema[i], i + 1);
		insert_SNL_byIndex(ListB, elemb[i], i + 1);
	}
	show_SNL(ListA);
	printf("\n");
	show_SNL(ListB);

	printf("\nInsert data-20 to index-5 in linkList_A: \n\t");
	insert_SNL_byIndex(ListA, 20, 5);
	show_SNL(ListA);

	index = location_SNL_byData(ListA, 13);
	printf("\nSearch data-13 in linkList_A: \n\tindex-%d", index);

	get_SNL_byIndex(ListA, 5, &item);
	printf("\nSearch index-5 in linkList_A: \n\tdata-%d", item);

	printf("\nDelete index-3 in linkList_B: \n\t");
	delete_SNL_byIndex(ListB, 3);
	show_SNL(ListB);

	printf("\nAfter sort the linklist: \n\t");
	sort_SNL_byData(ListA);
	sort_SNL_byData(ListB);
	show_SNL(ListA);
	printf("\n\t");
	show_SNL(ListB);

	printf("\nAfter merger two linklists: \n\t");
	merge_SNL_byData(ListA, ListB);
	show_SNL(ListA);

	printf("\nAfter reverse the linklist_A: \n\t");
	reverse_SNL_elem(ListA);
	show_SNL(ListA);

	clean_SNL(ListA);
	destroy_SNL(&ListA);
	printf("\nClear and Destory finish, quit!\n");

	return 0;
}
