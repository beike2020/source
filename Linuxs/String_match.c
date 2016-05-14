/******************************************************************************
 * Function:	A example of order string match operation.
 * Author: 	forwarding2012@yahoo.com.cn			  		 
 * Date:		2012.01.01				  		  
 * Complie:	gcc -Wall -g String_match.c -o String_match
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXSTRLEN 127
typedef char OMString[MAXSTRLEN + 1];

int assign_OMS(OMString T, char *chars)
{
	int i;

	if (strlen(chars) > MAXSTRLEN)
		return -1;

	T[0] = strlen(chars);
	for (i = 1; i <= T[0]; i++)
		T[i] = *(chars + i - 1);

	return 0;
}

void show_OMS(OMString T)
{
	int i;

	for (i = 1; i <= T[0]; i++)
		printf("%c", T[i]);
}

int getLength_OMS(OMString S)
{
	return S[0];
}

void getNext_KMP_A(OMString T, int next[])
{
	int i = 1, j = 0;

	next[1] = 0;
	while (i < T[0]) {
		if (j == 0 || T[i] == T[j])
			next[++i] = ++j;
		else
			j = next[j];
	}
}

void getNext_KMP_B(OMString T, int nextval[])
{
	int i = 1, j = 0;

	nextval[1] = 0;
	while (i < T[0]) {
		if (j == 0 || T[i] == T[j]) {
			if (T[++i] != T[++j])
				nextval[i] = j;
			else
				nextval[i] = nextval[j];
		} else {
			j = nextval[j];
		}
	}
}

int getIndex_KMP_A(OMString S, OMString T, int pos, int next[])
{
	int i = pos, j = 1;

	while (i <= S[0] && j <= T[0]) {
		if (j == 0 || S[i] == T[j]) {
			++i;
			++j;
		} else {
			j = next[j];
		}
	}

	if (j > T[0])
		return i - T[0];

	return 0;
}

int getIndex_KMP_B(OMString S, OMString T, int pos, int next[])
{
	int i = pos, j = 1;

	while (i <= S[0] && j <= T[0]) {
		if (j == 0 || S[i] == T[j]) {
			++i;
			++j;
		} else {
			j = next[j];
		}
	}

	if (j > T[0])
		return i - T[0];

	return 0;
}

int main()
{
	OMString a1, a2;
	int len, index, i, *p1, *p2;

	assign_OMS(a1, "acabaabaabcacaabc");
	assign_OMS(a2, "abaabcac");
	printf("\nString A1: ");
	show_OMS(a1);
	printf("\nSub String A2: ");
	show_OMS(a2);

	len = getLength_OMS(a2);
	p1 = (int *)malloc((len + 1) * sizeof(int));
	getNext_KMP_A(a2, p1);
	printf("\nThe next function[KMPA]:\n\t");
	for (i = 1; i <= len; i++)
		printf(" %d", *(p1 + i));

	index = getIndex_KMP_B(a1, a2, 1, p1);
	printf("\nFist match index A1 and A2[KMPA]: %d", index);
	free(p1);

	index = 0;
	p2 = (int *)malloc((len + 1) * sizeof(int));
	getNext_KMP_B(a2, p2);
	printf("\nThe next function[KMPB]:\n\t");
	for (i = 1; i <= len; i++)
		printf(" %d", *(p2 + i));

	index = getIndex_KMP_B(a1, a2, 1, p2);
	printf("\nFist match index A1 and A2[KMPB]: %d\n", index);
	free(p2);

	return 0;
}
