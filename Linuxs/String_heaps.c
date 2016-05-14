/******************************************************************************
 * Function:	A example of string saved by heap operation.
 * Author: 	forwarding2012@yahoo.com.cn			  		 
 * Date:		2012.01.01				  		  
 * Complie:	gcc -Wall -g String_heaps.c -o String_heaps
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	char *ch;
	int length;
} HSString;

void init_HSS(HSString * T)
{
	T->length = 0;
	T->ch = NULL;
}

int assign_HSS(HSString * T, char *chars)
{
	int i, j;

	if (T->ch)
		free(T->ch);

	if ((i = strlen(chars)) == 0) {
		T->ch = NULL;
		T->length = 0;
	} else {
		T->ch = (char *)malloc(i * sizeof(char));
		if (!T->ch)
			return -1;

		for (j = 0; j < i; j++)
			T->ch[j] = chars[j];
		T->length = i;
	}

	return 0;
}

int cleanContent_HSS(HSString * S)
{
	if (S->ch) {
		free(S->ch);
		S->ch = NULL;
	}

	S->length = 0;

	return 0;
}

void show_HSS(HSString T)
{
	int i;

	for (i = 0; i < T.length; i++)
		printf("%c", T.ch[i]);
}

int isEmpty_HSS(HSString S)
{
	if (S.length == 0 && S.ch == NULL)
		return 0;
	else
		return -1;
}

int getLength_HSS(HSString S)
{
	return S.length;
}

int copy_HSS(HSString * T, HSString S)
{
	int i;

	if (T->ch)
		free(T->ch);

	T->ch = (char *)malloc(S.length * sizeof(char));
	if (!T->ch)
		return -1;

	for (i = 0; i < S.length; i++)
		T->ch[i] = S.ch[i];
	T->length = S.length;

	return 0;
}

int compare_HSS(HSString S, HSString T)
{
	int i;

	for (i = 0; i < S.length && i < T.length; ++i)
		if (S.ch[i] != T.ch[i])
			return S.ch[i] - T.ch[i];

	return S.length - T.length;
}

int concat_HSS(HSString * T, HSString S1, HSString S2)
{
	int i;

	if (T->ch)
		free(T->ch);

	T->length = S1.length + S2.length;
	T->ch = (char *)malloc(T->length * sizeof(char));
	if (!T->ch)
		return -1;

	for (i = 0; i < S1.length; i++)
		T->ch[i] = S1.ch[i];

	for (i = 0; i < S2.length; i++)
		T->ch[S1.length + i] = S2.ch[i];

	return 0;
}

int getSub_HSS(HSString * Sub, HSString S, int pos, int len)
{
	int i;

	if (pos < 1 || pos > S.length || len < 0 || len > S.length - pos + 1)
		return -1;

	if ((*Sub).ch)
		free((*Sub).ch);

	if (!len) {
		(*Sub).ch = NULL;
		(*Sub).length = 0;
	} else {
		(*Sub).ch = (char *)malloc(len * sizeof(char));
		if (!(*Sub).ch)
			return -1;

		for (i = 0; i <= len - 1; i++)
			(*Sub).ch[i] = S.ch[pos - 1 + i];
		(*Sub).length = len;
	}

	return 0;
}

int locationSub_HSS(HSString S, HSString T, int pos)
{
	int n, m, i;
	HSString sub;

	init_HSS(&sub);

	if (pos > 0) {
		n = getLength_HSS(S);
		m = getLength_HSS(T);
		i = pos;
		while (i <= n - m + 1) {
			getSub_HSS(&sub, S, i, m);
			if (compare_HSS(sub, T) != 0)
				++i;
			else
				return i;
		}
	}

	return 0;
}

int insertSub_HSS(HSString * S, int pos, HSString T)
{
	int i;

	if (pos < 1 || pos > S->length + 1)
		return -1;

	if (T.length) {
		S->ch =
		    (char *)realloc(S->ch,
				    (S->length + T.length) * sizeof(char));

		if (!S->ch)
			return -1;

		for (i = S->length - 1; i >= pos - 1; --i)
			S->ch[i + T.length] = S->ch[i];

		for (i = 0; i < T.length; i++)
			S->ch[pos - 1 + i] = T.ch[i];

		S->length += T.length;
	}

	return 0;
}

int deleteSub_HSS(HSString * S, int pos, int len)
{
	int i;

	if (S->length < pos + len - 1)
		exit(-1);

	for (i = pos - 1; i <= S->length - len; i++)
		S->ch[i] = S->ch[i + len];

	S->length -= len;
	S->ch = (char *)realloc(S->ch, S->length * sizeof(char));

	return 0;
}

int replaceSub_HSS(HSString * S, HSString T, HSString V)
{
	int i = 1;

	if (isEmpty_HSS(T))
		return -1;

	do {
		i = locationSub_HSS(*S, T, i);
		if (i) {
			deleteSub_HSS(S, i, getLength_HSS(T));
			insertSub_HSS(S, i, V);
			i += getLength_HSS(V);
		}
	} while (i);

	return 0;
}

int main()
{
	int i;
	HSString t, s, r;
	char c, *p = "God bye!", *q = "God luck!";

	init_HSS(&t);
	init_HSS(&s);
	init_HSS(&r);

	assign_HSS(&t, p);
	printf("\nString T: ");
	show_HSS(t);
	printf("\n\t%d[0-empty], len: %d", isEmpty_HSS(t), getLength_HSS(t));

	assign_HSS(&s, q);
	printf("\nString S:\n\t");
	show_HSS(s);

	i = compare_HSS(s, t);
	if (i < 0)
		c = '<';
	else if (i == 0)
		c = '=';
	else
		c = '>';
	concat_HSS(&r, t, s);
	printf("\nString S %c String T to String R, then R:\n\t", c);
	show_HSS(r);

	assign_HSS(&s, "oo");
	printf("\nString S: ");
	show_HSS(s);
	assign_HSS(&t, "o");
	printf("\nString T: ");
	show_HSS(t);

	replaceSub_HSS(&r, t, s);
	printf("\nReplace String T in R to String S, then R:\n\t");
	show_HSS(r);

	cleanContent_HSS(&s);
	printf("\nClean S, then %d, len: %d", isEmpty_HSS(s), getLength_HSS(s));

	getSub_HSS(&s, r, 6, 4);
	printf("\nGet String S from String R[6,4], then S:\n\t");
	show_HSS(s);

	copy_HSS(&t, r);
	printf("\nCopy String T from String R, then T:\n\t");
	show_HSS(t);

	insertSub_HSS(&t, 6, s);
	printf("\nInsert string S to String T[6,*], then T:\n\t");
	show_HSS(t);

	deleteSub_HSS(&t, 1, 5);
	printf("\nDelete string from String T[1,5], then T:\n\t");
	show_HSS(t);

	printf("\nFirst com-string between String T[1,-] and S location: %d",
	       locationSub_HSS(t, s, 2));
	printf("\nFirst com-string between String T[2,-] and S location: %d\n",
	       locationSub_HSS(t, s, 2));

	return 0;
}
