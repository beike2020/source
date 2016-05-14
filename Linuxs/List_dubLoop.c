 /*****************************************************************************
  * Function:	A example of dub direct loop linklist operation.
  * Author: 	forwarding2012@yahoo.com.cn								 
  * Date: 	2012.01.01 	
  * Complie: 	gcc -Wall -g List_dupLoop.c -o List_dupLoop
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

typedef int ElemType;

typedef struct DLLNode {
	ElemType data;
	struct DLLNode *prev;
	struct DLLNode *next;
} DLLinkList;

int init_DLL(DLLinkList ** hNode)
{
	if ((*hNode = (DLLinkList *) malloc(sizeof(DLLinkList))) == NULL)
		return -1;

	(*hNode)->next = (*hNode)->prev = *hNode;

	return 0;
}

int clean_DLL(DLLinkList * hNode)
{
	DLLinkList *pNode, *pHold;

	pNode = hNode->next;
	while (pNode != hNode) {
		pHold = pNode->next;
		free(pNode);
		pNode = pHold;
	}

	hNode->next = hNode->prev = hNode;

	return 0;
}

int destroy_DLL(DLLinkList ** hNode)
{
	DLLinkList *pNode, *pHold;

	pNode = (*hNode)->next;
	while (pNode != *hNode) {
		pHold = pNode->next;
		free(pNode);
		pNode = pHold;
	}

	free(*hNode);
	*hNode = NULL;

	return 0;
}

int isEmpty_DLL(DLLinkList * hNode)
{
	if (hNode->next == hNode && hNode->prev == hNode)
		return 0;
	else
		return -1;
}

int getLength_DLL(DLLinkList * hNode)
{
	int index = 0;
	DLLinkList *pNode;

	pNode = hNode->next;
	while (pNode != hNode) {
		index++;
		pNode = pNode->next;
	}

	return index;
}

void show_DLL(DLLinkList * hNode)
{
	DLLinkList *pNode;

	pNode = hNode->next;
	while (pNode != hNode) {
		printf(" %d", pNode->data);
		pNode = pNode->next;
	}
}

int get_DLL_byIndex(DLLinkList * hNode, int dIndex, ElemType * data)
{
	int index = 1;
	DLLinkList *pNode;

	if (dIndex <= 0 || dIndex > getLength_DLL(hNode))
		return -1;

	pNode = hNode->next;
	while (index < dIndex) {
		index++;
		pNode = pNode->next;
	}

	*data = pNode->data;

	return 0;
}

int location_DLL_byData(DLLinkList * hNode, ElemType data)
{
	int index = 0;
	DLLinkList *pNode;

	pNode = hNode->next;
	while (pNode != hNode) {
		index++;
		if (pNode->data == data)
			return index;
		pNode = pNode->next;
	}

	return 0;
}

int getPrev_DLL_byData(DLLinkList * hNode, ElemType cur, ElemType * prev)
{
	DLLinkList *pNode;

	pNode = hNode->next->next;
	while (pNode != hNode) {
		if (pNode->data == cur) {
			*prev = pNode->prev->data;
			return 0;
		}
		pNode = pNode->next;
	}

	return -1;
}

int getNext_DLL_byData(DLLinkList * hNode, ElemType cur, ElemType * next)
{
	DLLinkList *pNode;

	pNode = hNode->next->next;
	while (pNode != hNode) {
		if (pNode->prev->data == cur) {
			*next = pNode->data;
			return 0;
		}
		pNode = pNode->next;
	}

	return -1;
}

int insert_DLL_byIndex(DLLinkList * hNode, ElemType data, int dIndex)
{
	int index;
	DLLinkList *pHold, *pNode;

	if (dIndex < 1 || dIndex > getLength_DLL(hNode) + 1)
		return -1;

	pNode = hNode;
	for (index = 1; index < dIndex; index++)
		pNode = pNode->next;

	if ((pHold = (DLLinkList *) malloc(sizeof(DLLinkList))) == NULL)
		return -1;

	pHold->data = data;
	pHold->prev = pNode;
	pHold->next = pNode->next;
	pNode->next->prev = pHold;
	pNode->next = pHold;

	return 0;
}

int delete_DLL_byIndex(DLLinkList * hNode, int dIndex)
{
	int index;
	DLLinkList *pNode;

	if (dIndex < 1 || dIndex > getLength_DLL(hNode))
		return -1;

	pNode = hNode;
	for (index = 1; index <= dIndex; index++)
		pNode = pNode->next;

	if (pNode == NULL)
		return -1;

	pNode->prev->next = pNode->next;
	pNode->next->prev = pNode->prev;
	free(pNode);

	return 0;
}

void sort_DLL_byData(DLLinkList * hNode)
{
	ElemType data;
	DLLinkList *pNode, *pHold;

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

void merge_DLL_byData(DLLinkList * LA, DLLinkList * LB)
{
	DLLinkList *r, *pHold, *pNode;

	r = LA;
	pHold = LA->next;
	pNode = LB->next;
	r->next = r->prev = NULL;

	while (pHold != LA && pNode != LB) {
		if (pHold->data <= pNode->data) {
			r->next = pHold;
			pHold->prev = r;
			r = pHold;
			pHold = pHold->next;
		} else {
			r->next = pNode;
			pNode->prev = r;
			r = pNode;
			pNode = pNode->next;
		}
	}

	if (pHold != LA) {
		r->next = pHold;
		pHold->prev = r;
	} else if (pNode != LB) {
		while (pNode != LB) {
			r->next = pNode;
			pNode->prev = r;
			r = pNode;
			pNode = pNode->next;
		}
		r->next = LA;
		LA->prev = r;
	}
	free(LB);
	return;
}

int main()
{
	int i, index;
	ElemType item;
	DLLinkList *ListA, *ListB;
	ElemType elema[10] = { 16, 8, 23, 2, 31, 13, 19, 87, 7, 24 };
	ElemType elemb[10] = { 6, 23, 22, 9, 1, 67, 15, 20, 9, 3 };

	init_DLL(&ListA);
	init_DLL(&ListB);
	for (i = 0; i < 10; i++) {
		insert_DLL_byIndex(ListA, elema[i], i + 1);
		insert_DLL_byIndex(ListB, elemb[i], i + 1);
	}
	show_DLL(ListA);
	printf("\n");
	show_DLL(ListB);

	printf("\nInsert data-20 to index-5 in linkList_A: \n\t");
	insert_DLL_byIndex(ListA, 20, 5);
	show_DLL(ListA);

	index = location_DLL_byData(ListA, 13);
	printf("\nSearch data-13 in linkList_A: \n\tindex-%d", index);

	get_DLL_byIndex(ListA, 5, &item);
	printf("\nSearch index-5 in linkList_A: \n\tdata-%d", item);

	printf("\nDelete index-3 in linkList_B: \n\t");
	delete_DLL_byIndex(ListB, 3);
	show_DLL(ListB);

	printf("\nAfter sort the linklist: \n\t");
	sort_DLL_byData(ListA);
	sort_DLL_byData(ListB);
	show_DLL(ListA);
	printf("\n\t");
	show_DLL(ListB);

	printf("\nAfter merger two linklists: \n\t");
	merge_DLL_byData(ListA, ListB);
	show_DLL(ListA);

	clean_DLL(ListA);
	destroy_DLL(&ListA);
	printf("\nClear and Destory finish, quit!\n");

	return 0;
}
