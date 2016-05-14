/******************************************************************************
 * Function:	A example of avl tree operation.
 * Author: 	forwarding2012@yahoo.com.cn			  		 
 * Date:		2012.01.01				  		  
 * Complie:	gcc -Wall BiTree_avls.c -o BiTree_avls
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct BBTNode {
	unsigned int nData;
	int nHeight;
	struct BBTNode *pLeft;
	struct BBTNode *pRight;
} BBTree;

int Max(int a, int b)
{
	return (a > b ? a : b);
}

int Height(BBTree * pNode)
{
	if (NULL == pNode)
		return -1;

	return pNode->nHeight;
}

BBTree *SingleRotateWithLeft(BBTree * pNode)
{
	BBTree *pNode1;

	pNode1 = pNode->pLeft;
	pNode->pLeft = pNode1->pRight;
	pNode1->pRight = pNode;
	pNode->nHeight = Max(Height(pNode->pLeft), Height(pNode->pRight)) + 1;
	pNode1->nHeight = Max(Height(pNode1->pLeft), pNode->nHeight) + 1;

	return pNode1;
}

BBTree *SingleRotateWithRight(BBTree * pNode)
{
	BBTree *pNode1;

	pNode1 = pNode->pRight;
	pNode->pRight = pNode1->pLeft;
	pNode1->pLeft = pNode;
	pNode->nHeight = Max(Height(pNode->pLeft), Height(pNode->pRight)) + 1;
	pNode1->nHeight = Max(Height(pNode1->pRight), pNode->nHeight) + 1;

	return pNode1;
}

BBTree *DoubleRotateWithLeft(BBTree * pNode)
{
	pNode->pLeft = SingleRotateWithRight(pNode->pLeft);

	return SingleRotateWithLeft(pNode);
}

BBTree *DoubleRotateWithRight(BBTree * pNode)
{
	pNode->pRight = SingleRotateWithLeft(pNode->pRight);

	return SingleRotateWithRight(pNode);
}

BBTree *insert_tree(unsigned int nData, BBTree * pNode)
{
	if (NULL == pNode) {
		pNode = (BBTree *) malloc(sizeof(BBTree));
		pNode->nData = nData;
		pNode->nHeight = 0;
		pNode->pLeft = pNode->pRight = NULL;
	} else if (nData < pNode->nData) {
		pNode->pLeft = insert_tree(nData, pNode->pLeft);
		if (Height(pNode->pLeft) - Height(pNode->pRight) == 2) {
			if (nData < pNode->pLeft->nData)
				pNode = SingleRotateWithLeft(pNode);
			else
				pNode = DoubleRotateWithLeft(pNode);
		}
	} else if (nData > pNode->nData) {
		pNode->pRight = insert_tree(nData, pNode->pRight);
		if (Height(pNode->pRight) - Height(pNode->pLeft) == 2) {
			if (nData > pNode->pRight->nData)
				pNode = SingleRotateWithRight(pNode);
			else
				pNode = DoubleRotateWithRight(pNode);
		}
	}

	pNode->nHeight = Max(Height(pNode->pLeft), Height(pNode->pRight)) + 1;

	return pNode;
}

void delete_tree(BBTree ** ppRoot)
{
	if (NULL == ppRoot || NULL == *ppRoot)
		return;

	delete_tree(&((*ppRoot)->pLeft));
	delete_tree(&((*ppRoot)->pRight));
	free(*ppRoot);
	*ppRoot = NULL;
}

void print_tree(BBTree * pRoot)
{
	static int n = 0;

	if (NULL == pRoot)
		return;

	print_tree(pRoot->pLeft);
	printf("[%d]nData = %u\n", ++n, pRoot->nData);
	print_tree(pRoot->pRight);
}

int find_tree(unsigned int data, BBTree * pRoot)
{
	static int k = 1;

	if (pRoot == NULL) {
		printf("not find %d times\n", k);
		return 0;
	}

	if (data == pRoot->nData) {
		printf("find:%d times\n", k);
		return 1;
	} else if (data < pRoot->nData) {
		++k;
		return find_tree(data, pRoot->pLeft);
	} else if (data > pRoot->nData) {
		++k;
		return find_tree(data, pRoot->pRight);
	}

	return 0;
}

int main()
{
	int i, j;
	BBTree *pRoot;

	srand((unsigned int)time(NULL));

	for (i = 0; i < 10; ++i) {
		j = rand();
		printf("%d\n", j);
		pRoot = insert_tree(j, pRoot);
	}

	print_tree(pRoot);

	delete_tree(&pRoot);

	return 0;
}
