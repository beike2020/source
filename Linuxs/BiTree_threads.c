/******************************************************************************
 * Function:	A tree saved by tags operation
 * Author:	forwarding2012@yahoo.com.cn
 * Date:		2012.01.01
 * Compile:	gcc -Wall BiTree_threads.c -o BiTree_threads
******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef int ElemType;
typedef enum { Link, Thread } PointerTag;
typedef struct TBTNode {
	ElemType data;
	struct TBTNode *lchild;
	struct TBTNode *rchild;
	PointerTag LTag;
	PointerTag RTag;
} TBiTree;

TBiTree *pre;
ElemType elem[10] = { 6, 23, 22, 9, 1, 67, 15, 20, 9, 3 };
int indexs = 0;

int creat_TBT(TBiTree ** T)
{
	*T = (TBiTree *) malloc(sizeof(TBiTree));
	if (*T == NULL || indexs > 9)
		return -1;

	(*T)->data = elem[indexs++];
	creat_TBT(&(*T)->lchild);
	if ((*T)->lchild)
		(*T)->LTag = Link;

	creat_TBT(&(*T)->rchild);
	if ((*T)->rchild)
		(*T)->RTag = Link;

	return 0;
}

void inOrderTraverse_TBT(TBiTree * p)
{
	if (p == NULL)
		return;

	inOrderTraverse_TBT(p->lchild);
	if (!p->lchild) {
		p->LTag = Thread;
		p->lchild = pre;
	}

	if (!pre->rchild) {
		pre->RTag = Thread;
		pre->rchild = p;
	}

	pre = p;
	inOrderTraverse_TBT(p->rchild);
}

int inOrderThreading_TBT(TBiTree * *Thrt, TBiTree * T)
{
	*Thrt = (TBiTree *) malloc(sizeof(TBiTree));
	if (*Thrt == NULL)
		return -1;

	(*Thrt)->LTag = Link;
	(*Thrt)->RTag = Thread;
	(*Thrt)->rchild = *Thrt;
	if (T == NULL) {
		(*Thrt)->lchild = *Thrt;
	} else {
		(*Thrt)->lchild = T;
		pre = *Thrt;
		inOrderTraverse_TBT(T);
		pre->rchild = *Thrt;
		pre->RTag = Thread;
		(*Thrt)->rchild = pre;
	}

	return 0;
}

int InOrderTraverseThreading_TBT(TBiTree * T)
{
	TBiTree *p;

	p = T->lchild;
	while (p != T) {
		while (p->LTag == Link)
			p = p->lchild;

		while (p->RTag == Thread && p->rchild != T)
			p = p->rchild;

		printf("%d ", p->data);
		p = p->rchild;
	}

	return 0;
}

int main()
{
	TBiTree *H, *T;

	creat_TBT(&T);
	inOrderThreading_TBT(&H, T);

	printf("\nBiTree InOrder TraverseThreading:\n");
	InOrderTraverseThreading_TBT(H);
	printf("\n");

	return 0;
}
