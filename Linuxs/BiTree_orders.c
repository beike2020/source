/******************************************************************************
 * Function:	A example of bit tree saved by order table operation.
 * Author:	forwarding2012@yahoo.com.cn				  
 * Date:		2012.01.01 					   
 * Complie: 	gcc -Wall BiTree_orders.c -lm -o BiTree_orders
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define MAX_SIZE 100

typedef int ElemType;
typedef ElemType OBiTree[MAX_SIZE];

typedef struct {
	int level;
	int order;
} Position;

int init_OBT(OBiTree T)
{
	int i;

	for (i = 0; i < MAX_SIZE; i++)
		T[i] = 0;

	return 0;
}

int clean_OBT(OBiTree T)
{
	int i;

	for (i = 0; i < MAX_SIZE; i++)
		T[i] = 0;

	return 0;
}

int create_OBT(OBiTree T)
{
	int i = 0;

	while (i < 10) {
		T[i] = i + 1;
		if (i != 0 && T[(i + 1) / 2 - 1] == 0 && T[i] != 0)
			return -1;
		i++;
	}

	while (i < MAX_SIZE)
		T[i++] = 0;

	return 0;
}

int isEmpty_OBT(OBiTree T)
{
	if (T[0] == 0)
		return 1;
	else
		return 0;
}

int getDepth_OBT(OBiTree T)
{
	int i, j = -1;

	for (i = MAX_SIZE - 1; i >= 0; i--) {
		if (T[i] != 0)
			break;
	}

	i++;
	do {
		j++;
	} while (i >= pow(2, j));

	return j;
}

int getRoot_OBT(OBiTree T, ElemType * e)
{
	if (isEmpty_OBT(T)) {
		return -1;
	} else {
		*e = T[0];
		return 0;
	}
}

ElemType getValue_OBT(OBiTree T, Position e)
{
	return T[(int)pow(2, e.level - 1) + e.order - 2];
}

int assign_OBT(OBiTree T, Position e, ElemType value)
{
	int i = (int)pow(2, e.level - 1) + e.order - 2;

	if (value != 0 && T[(i + 1) / 2 - 1] == 0)
		return -1;
	else if (value == 0 && (T[i * 2 + 1] != 0 || T[i * 2 + 2] != 0))
		return -1;

	T[i] = value;

	return 0;
}

ElemType getParent_OBT(OBiTree T, ElemType e)
{
	int i;

	if (T[0] == 0)
		return 0;

	for (i = 1; i <= MAX_SIZE - 1; i++) {
		if (T[i] == e)
			return T[(i + 1) / 2 - 1];
	}

	return 0;
}

ElemType getLChild_OBT(OBiTree T, ElemType e)
{
	int i;

	if (T[0] == 0)
		return 0;

	for (i = 0; i <= MAX_SIZE - 1; i++) {
		if (T[i] == e)
			return T[i * 2 + 1];
	}

	return 0;
}

ElemType getRChild_OBT(OBiTree T, ElemType e)
{
	int i;

	if (T[0] == 0)
		return 0;

	for (i = 0; i <= MAX_SIZE - 1; i++) {
		if (T[i] == e)
			return T[i * 2 + 2];
	}

	return 0;
}

ElemType getLBrother_OBT(OBiTree T, ElemType e)
{
	int i;

	if (T[0] == 0)
		return 0;

	for (i = 1; i <= MAX_SIZE - 1; i++) {
		if (T[i] == e && i % 2 == 0)
			return T[i - 1];
	}

	return 0;
}

ElemType getRBrother_OBT(OBiTree T, ElemType e)
{
	int i;

	if (T[0] == 0)
		return 0;

	for (i = 1; i <= MAX_SIZE - 1; i++) {
		if (T[i] == e && i % 2)
			return T[i + 1];
	}

	return 0;
}

void preTraverse_OBT(OBiTree T, int e)
{
	printf("%d ", T[e]);

	if (T[2 * e + 1] != 0)
		preTraverse_OBT(T, 2 * e + 1);

	if (T[2 * e + 2] != 0)
		preTraverse_OBT(T, 2 * e + 2);
}

int preOrderTraverse_OBT(OBiTree T)
{
	if (!isEmpty_OBT(T))
		preTraverse_OBT(T, 0);

	return 0;
}

void inTraverse_OBT(OBiTree T, int e)
{
	if (T[2 * e + 1] != 0)
		inTraverse_OBT(T, 2 * e + 1);
	printf("%d ", T[e]);

	if (T[2 * e + 2] != 0)
		inTraverse_OBT(T, 2 * e + 2);
}

int inOrderTraverse_OBT(OBiTree T)
{
	if (!isEmpty_OBT(T))
		inTraverse_OBT(T, 0);

	return 0;
}

void postTraverse_OBT(OBiTree T, int e)
{
	if (T[2 * e + 1] != 0)
		postTraverse_OBT(T, 2 * e + 1);

	if (T[2 * e + 2] != 0)
		postTraverse_OBT(T, 2 * e + 2);
	printf("%d ", T[e]);
}

int postOrderTraverse_OBT(OBiTree T)
{
	if (!isEmpty_OBT(T))
		postTraverse_OBT(T, 0);

	return 0;
}

void levelOrderTraverse_OBT(OBiTree T)
{
	int i = MAX_SIZE - 1, j;

	while (T[i] == 0)
		i--;

	for (j = 0; j <= i; j++) {
		if (T[j] != 0)
			printf("%d ", T[j]);
	}
}

int main()
{
	int i, ret, len;
	Position p;
	ElemType e;
	OBiTree T;

	init_OBT(T);
	create_OBT(T);
	ret = isEmpty_OBT(T);
	len = getDepth_OBT(T);
	i = getRoot_OBT(T, &e);
	printf("\nBiTree empty? %d[0-NO], depth: %d", ret, len);
	printf("\nBiTree root node: %d[0-empty]", e);

	printf("\nBiTree levelOrderTraverse:\n\t");
	levelOrderTraverse_OBT(T);
	printf("\nBiTree preOrderTraverse:\n\t");
	preOrderTraverse_OBT(T);
	printf("\nBiTree inOrderTraverse:\n\t");
	inOrderTraverse_OBT(T);
	printf("\nBiTree postOrderTraverse:\n\t");
	postOrderTraverse_OBT(T);

	p.level = 2;
	p.order = 2;
	e = getValue_OBT(T, p);
	printf("\nBiTree Node[3,2]: %d", e);
	e = 50;
	assign_OBT(T, p, e);
	printf(", then set to 50, preOrderTraverse:\n\t");
	preOrderTraverse_OBT(T);

	printf("\nBiTree node-%d's parent: %d", e, getParent_OBT(T, e));
	printf("\nBiTree node-%d's LChild: %d", e, getLChild_OBT(T, e));
	printf("\nBiTree node-%d's RChild: %d", e, getRChild_OBT(T, e));
	printf("\nBiTree node-%d's LBrother: %d", e, getLBrother_OBT(T, e));
	printf("\nBiTree node-%d's RBrother: %d\n", e, getRBrother_OBT(T, e));

	clean_OBT(T);

	return 0;
}
