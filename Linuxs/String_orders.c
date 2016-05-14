/******************************************************************************
 * Function:	A example of string saved by order operation.
 * Author: 	forwarding2012@yahoo.com.cn			  		 
 * Date:		2012.01.01				  		  
 * Complie:	gcc -Wall -g String_orders.c -o String_orders
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define  MAXSTR	127
typedef char OSString[MAXSTR + 1];

int assign_OSS(OSString T, char *chars)
{
	int i;

	if (strlen(chars) > MAXSTR)
		return -1;

	T[0] = strlen(chars);
	for (i = 1; i <= T[0]; i++)
		T[i] = chars[i - 1];

	return 0;
}

int cleanContent_OSS(OSString S)
{
	S[0] = 0;

	return 0;
}

void show_OSS(OSString T)
{
	int i;

	for (i = 1; i <= T[0]; i++)
		printf("%c", T[i]);
}

int isEmpty_OSS(OSString S)
{
	if (S[0] == 0)
		return 0;
	else
		return -1;
}

int getLength_OSS(OSString S)
{
	return S[0];
}

int copy_OSS(OSString T, OSString S)
{
	int i;

	for (i = 0; i <= S[0]; i++)
		T[i] = S[i];

	return 0;
}

int compare_OSS(OSString S, OSString T)
{
	int i;

	for (i = 1; i <= S[0] && i <= T[0]; ++i) {
		if (S[i] != T[i])
			return S[i] - T[i];
	}

	return S[0] - T[0];
}

int concat_OSS(OSString T, OSString S1, OSString S2)
{
	int i;

	if (S1[0] + S2[0] <= MAXSTR) {
		for (i = 1; i <= S1[0]; i++)
			T[i] = S1[i];

		for (i = 1; i <= S2[0]; i++)
			T[S1[0] + i] = S2[i];
		T[0] = S1[0] + S2[0];

		return 0;
	} else {
		for (i = 1; i <= S1[0]; i++)
			T[i] = S1[i];

		for (i = 1; i <= MAXSTR - S1[0]; i++)
			T[S1[0] + i] = S2[i];
		T[0] = MAXSTR;

		return -1;
	}
}

int getSub_OSS(OSString Sub, OSString S, int pos, int len)
{
	int i;

	if (pos < 1 || pos > S[0] || len < 0 || len > S[0] - pos + 1)
		return -1;

	for (i = 1; i <= len; i++)
		Sub[i] = S[pos + i - 1];
	Sub[0] = len;

	return 0;
}

int locationSub_OSS(OSString S, OSString T, int pos)
{
	int i, j;

	if (1 <= pos && pos <= S[0]) {
		i = pos;
		j = 1;
		while (i <= S[0] && j <= T[0]) {
			if (S[i] == T[j]) {
				++i;
				++j;
			} else {
				i = i - j + 2;
				j = 1;
			}
		}

		if (j > T[0])
			return i - T[0];
		else
			return 0;
	}

	return 0;
}

int insertSub_OSS(OSString S, int pos, OSString T)
{
	int i;

	if (pos < 1 || pos > S[0] + 1)
		return -1;

	if (S[0] + T[0] <= MAXSTR) {
		for (i = S[0]; i >= pos; i--)
			S[i + T[0]] = S[i];

		for (i = pos; i < pos + T[0]; i++)
			S[i] = T[i - pos + 1];
		S[0] = S[0] + T[0];

		return 0;
	} else {
		for (i = MAXSTR; i <= pos; i--)
			S[i] = S[i - T[0]];

		for (i = pos; i < pos + T[0]; i++)
			S[i] = T[i - pos + 1];
		S[0] = MAXSTR;

		return -1;
	}
}

int deleteSub_OSS(OSString S, int pos, int len)
{
	int i;

	if (pos < 1 || pos > S[0] - len + 1 || len < 0)
		return -1;

	for (i = pos + len; i <= S[0]; i++)
		S[i - len] = S[i];
	S[0] -= len;

	return 0;
}

int replaceSub_OSS(OSString S, OSString T, OSString V)
{
	int i = 1;

	if (isEmpty_OSS(T))
		return -1;

	do {
		if ((i = locationSub_OSS(S, T, i))) {
			deleteSub_OSS(S, i, getLength_OSS(T));
			insertSub_OSS(S, i, V);
			i += getLength_OSS(V);
		}
	} while (i);

	return 0;
}

int main()
{
	char s;
	int i;
	OSString t, s1, s2;

	assign_OSS(s1, "God bye!");
	printf("\nString S1: ");
	show_OSS(s1);
	printf("\n\t%d[0-empty], len: %d", isEmpty_OSS(s1), getLength_OSS(s1));
	copy_OSS(s2, s1);
	printf("\nCopy String S2 from String S1, then S2:\n\t");
	show_OSS(s2);

	assign_OSS(s2, "God luck!");
	i = compare_OSS(s1, s2);
	if (i < 0)
		s = '<';
	else if (i == 0)
		s = '=';
	else
		s = '>';
	printf("\nString S1 %c String S2", s);

	concat_OSS(t, s1, s2);
	printf("\nString S1 concat String S2 to String T, then T:\n\t");
	show_OSS(t);

	cleanContent_OSS(s1);
	printf("\nString S1: ");
	show_OSS(s1);
	printf("\n\t%d[0-empty], len: %d", isEmpty_OSS(s1), getLength_OSS(s1));

	getSub_OSS(s2, t, 2, 2);
	printf("\nGet String S2 from String T[2,2], then S2:\n\t");
	show_OSS(s2);

	deleteSub_OSS(t, 1, 3);
	printf("\nDelete string from String T[1,3], then T:\n\t");
	show_OSS(t);

	i = getLength_OSS(s2) / 2;
	insertSub_OSS(s2, 3, t);
	printf("\nInsert string S2 to String T[3,*], then S2:\n\t");
	show_OSS(s2);

	i = locationSub_OSS(s2, t, 1);
	printf("\nFirst com-string between String T and S2 location: %d", i);

	getSub_OSS(t, s2, 1, 1);
	printf("\nGet String S2 from String S2[1,1], then T:\n\t");
	show_OSS(t);

	concat_OSS(s1, t, t);
	printf("\nConcat string T and T to S1, then S1:\n\t");
	show_OSS(s1);

	replaceSub_OSS(s2, t, s1);
	printf("\nReplace String S1 in S2 comm to T, then S2:\n\t");
	show_OSS(s2);
	printf("\n");

	return 0;
}
