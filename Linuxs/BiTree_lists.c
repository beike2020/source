/******************************************************************************
 * Function:	A example of bit tree list operation.
 * Author: 	forwarding2012@yahoo.com.cn			  		 
 * Date:		2012.01.01				  		  
 * Complie:	gcc -Wall BiTree_lists.c -o BiTree_lists
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>

typedef char ElemType;

typedef struct LBTNode {
	ElemType data;
	struct LBTNode *lchild;
	struct LBTNode *rchild;
} LBiTree;

LBiTree *init_LBT(ElemType data)
{
	LBiTree *pNode;

	if ((pNode = (LBiTree *) malloc(sizeof(LBiTree))) == NULL)
		return NULL;

	pNode->data = data;
	pNode->lchild = pNode->rchild = NULL;

	return pNode;
}

LBiTree *insert_LBT(LBiTree * p, ElemType data)
{
	if (p == NULL)
		p = init_LBT(data);
	else if (p->data > data)
		p->lchild = insert_LBT(p->lchild, data);
	else
		p->rchild = insert_LBT(p->rchild, data);

	return p;
}

void preOrderTraverse_LBT(LBiTree * p)
{
	if (p != NULL) {
		printf(" %d", p->data);
		preOrderTraverse_LBT(p->lchild);
		preOrderTraverse_LBT(p->rchild);
	}
}

void inOrderTraverse_LBT(LBiTree * p)
{
	if (p != NULL) {
		inOrderTraverse_LBT(p->lchild);
		printf(" %d", p->data);
		inOrderTraverse_LBT(p->rchild);
	}
}

void postOrderTraverse_LBT(LBiTree * p)
{
	if (p != NULL) {
		postOrderTraverse_LBT(p->lchild);
		postOrderTraverse_LBT(p->rchild);
		printf(" %d", p->data);
	}
}

void countLeaf_LBT(LBiTree * p, int *count)
{
	if (p != NULL) {
		if (p->lchild == NULL && p->rchild == NULL)
			*count = *count + 1;
		countLeaf_LBT(p->lchild, count);
		countLeaf_LBT(p->rchild, count);
	}
}

int getHigh_LBT(LBiTree * p)
{
	int lh, rh, mh = 0;

	if (p != NULL) {
		lh = getHigh_LBT(p->lchild);
		rh = getHigh_LBT(p->rchild);
		mh = (lh > rh ? lh : rh) + 1;
	}

	return mh;
}

int main()
{
	LBiTree *tree;
	int tmp = 0, high = 0, loop = 0, count = 0;

	if ((tree = init_LBT(0)) == NULL)
		return -1;

	printf("BitTree insert random data: 0 ");
	for (loop = 0; loop < 9; loop++) {
		tmp = rand() % 100;
		insert_LBT(tree, tmp);
		printf("%d ", tmp);
	}

	printf("\nBitTree reverse by PreOrder: ");
	preOrderTraverse_LBT(tree);

	printf("\nBitTree reverse by InOrder:  ");
	inOrderTraverse_LBT(tree);

	printf("\nBitTree reverse by PostOrder: ");
	postOrderTraverse_LBT(tree);

	countLeaf_LBT(tree, &count);
	high = getHigh_LBT(tree);
	printf("\nBitTree has %d leaves, high: %d\n", count, high);

	return 0;
}
