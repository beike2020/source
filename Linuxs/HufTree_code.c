/******************************************************************************
 * Function:	Huffman code operation
 * Author:	forwarding2012@yahoo.com.cn
 * Date:		2012.01.01
 * Compile:	gcc -Wall HufTree_code.c -o HufTree_code
******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define  WEIGH_NUM	9

typedef struct {
	unsigned int weight;
	unsigned int parent;
	unsigned int lchild;
	unsigned int rchild;
} HFTree;

typedef char **HFCode;

int getMin_HFT(HFTree * t, int i)
{
	int j, flag;
	unsigned int k = 65536;

	for (j = 1; j <= i; j++) {
		if (t[j].weight < k && t[j].parent == 0)
			k = t[j].weight, flag = j;
	}
	t[flag].parent = 1;

	return flag;
}

void selectMin_HFT(HFTree * t, int i, int *s1, int *s2)
{
	int tmp;

	*s1 = getMin_HFT(t, i);
	*s2 = getMin_HFT(t, i);
	if (*s1 > *s2) {
		tmp = *s1;
		*s1 = *s2;
		*s2 = tmp;
	}
}

void codeA_HFT(HFTree * *HT, HFCode * HC, int *w, int n)
{
	char *cd;
	HFTree *p;
	unsigned int c, f;
	int m, i, s1, s2, start;

	if (n <= 1)
		return;

	m = 2 * n - 1;
	*HT = (HFTree *) malloc((m + 1) * sizeof(HFTree));
	for (p = *HT + 1, i = 1; i <= n; ++i, ++p, ++w) {
		p->weight = *w;
		p->parent = 0;
		p->lchild = 0;
		p->rchild = 0;
	}

	for (; i <= m; ++i, ++p)
		p->parent = 0;

	for (i = n + 1; i <= m; ++i) {
		selectMin_HFT(*HT, i - 1, &s1, &s2);
		(*HT)[s1].parent = (*HT)[s2].parent = i;
		(*HT)[i].lchild = s1;
		(*HT)[i].rchild = s2;
		(*HT)[i].weight = (*HT)[s1].weight + (*HT)[s2].weight;
	}

	*HC = (HFCode) malloc((n + 1) * sizeof(char *));
	cd = (char *)malloc(n * sizeof(char));
	cd[n - 1] = '\0';

	for (i = 1; i <= n; i++) {
		start = n - 1;
		for (c = i, f = (*HT)[i].parent; f; c = f, f = (*HT)[f].parent) {
			if ((*HT)[f].lchild == c)
				cd[--start] = '0';
			else
				cd[--start] = '1';
		}

		(*HC)[i] = (char *)malloc((n - start) * sizeof(char));
		strcpy((*HC)[i], &cd[start]);
	}

	free(cd);
}

void codeB_HFT(HFTree * *HT, HFCode * HC, int *w, int n)
{
	char *cd;
	HFTree *p;
	int m, i, s1, s2;
	unsigned int c, cdlen;

	if (n <= 1)
		return;

	m = 2 * n - 1;
	*HT = (HFTree *) malloc((m + 1) * sizeof(HFTree));
	for (p = *HT + 1, i = 1; i <= n; ++i, ++p, ++w) {
		p->weight = *w;
		p->parent = 0;
		p->lchild = 0;
		p->rchild = 0;
	}

	for (; i <= m; ++i, ++p)
		p->parent = 0;

	for (i = n + 1; i <= m; ++i) {
		selectMin_HFT(*HT, i - 1, &s1, &s2);
		(*HT)[s1].parent = (*HT)[s2].parent = i;
		(*HT)[i].lchild = s1;
		(*HT)[i].rchild = s2;
		(*HT)[i].weight = (*HT)[s1].weight + (*HT)[s2].weight;
	}

	*HC = (HFCode) malloc((n + 1) * sizeof(char *));
	cd = (char *)malloc(n * sizeof(char));
	c = m;
	cdlen = 0;

	for (i = 1; i <= m; ++i)
		(*HT)[i].weight = 0;

	while (c) {
		if ((*HT)[c].weight == 0) {
			(*HT)[c].weight = 1;
			if ((*HT)[c].lchild != 0) {
				c = (*HT)[c].lchild;
				cd[cdlen++] = '0';
			} else if ((*HT)[c].rchild == 0) {
				(*HC)[c] =
				    (char *)malloc((cdlen + 1) * sizeof(char));
				cd[cdlen] = '\0';
				strcpy((*HC)[c], cd);
			}
		} else if ((*HT)[c].weight == 1) {
			(*HT)[c].weight = 2;
			if ((*HT)[c].rchild != 0) {
				c = (*HT)[c].rchild;
				cd[cdlen++] = '1';
			}
		} else {
			(*HT)[c].weight = 0;
			c = (*HT)[c].parent;
			--cdlen;
		}
	}

	free(cd);
}

int main()
{
	HFCode HC;
	HFTree *HT;
	int *w, n, i;

	w = (int *)malloc(WEIGH_NUM * sizeof(int));
	for (i = 0; i <= WEIGH_NUM - 1; i++)
		w[i] = i + 2;

	printf("\nUse Huffman code A:\n");
	codeA_HFT(&HT, &HC, w, n);
	for (i = 1; i <= WEIGH_NUM; i++)
		puts(HC[i]);

	printf("\nUse Huffman code B:\n");
	codeB_HFT(&HT, &HC, w, n);
	for (i = 1; i <= WEIGH_NUM; i++)
		puts(HC[i]);

	free(w);

	return 0;
}
