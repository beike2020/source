/******************************************************************************
 * Function:	A example of string saved by list operation.
 * Author: 	forwarding2012@yahoo.com.cn			  		 
 * Date:		2012.01.01		  		  
 * Complie:	gcc -Wall -g String_lists.c -o String_lists
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define  CHUNKSIZE 4

typedef struct Chunk {
	char ch[CHUNKSIZE];
	struct Chunk *next;
} Chunk;

typedef struct {
	Chunk *head, *tail;
	int curlen;
} LSString;

void init_LSS(LSString * T)
{
	T->curlen = 0;
	T->head = T->tail = NULL;
}

int assign_LSS(LSString * T, char *chars)
{
	int i, j, k, l;
	Chunk *p, *q;

	i = strlen(chars);
	if (!i || strchr(chars, ' '))
		return -1;

	T->curlen = i;
	j = i / CHUNKSIZE;
	if (i % CHUNKSIZE)
		j++;

	for (k = 0; k < j; k++) {
		p = (Chunk *) malloc(sizeof(Chunk));
		if (!p)
			return -1;

		if (k == 0) {
			T->head = q = p;
		} else {
			q->next = p;
			q = p;
		}

		for (l = 0; l < CHUNKSIZE && *chars; l++)
			*(q->ch + l) = *chars++;

		if (!*chars) {
			T->tail = q;
			q->next = NULL;
			for (; l < CHUNKSIZE; l++)
				*(q->ch + l) = ' ';
		}
	}

	return 0;
}

int cleanContent_LSS(LSString * S)
{
	Chunk *p, *q;

	p = S->head;
	while (p) {
		q = p->next;
		free(p);
		p = q;
	}

	S->head = S->tail = NULL;
	S->curlen = 0;

	return 0;
}

int isEmpty_LSS(LSString S)
{
	if (S.curlen)
		return -1;
	else
		return 0;
}

int getLength_LSS(LSString S)
{
	return S.curlen;
}

int copy_LSS(LSString * T, LSString S)
{
	Chunk *h = S.head, *p, *q;

	T->curlen = S.curlen;
	if (h == NULL)
		return -1;

	p = T->head = (Chunk *) malloc(sizeof(Chunk));
	*p = *h;
	h = h->next;
	while (h) {
		q = p;
		p = (Chunk *) malloc(sizeof(Chunk));
		q->next = p;
		*p = *h;
		h = h->next;
	}
	p->next = NULL;
	T->tail = p;

	return 0;
}

int compare_LSS(LSString S, LSString T)
{
	int i = 0;
	Chunk *ps = S.head, *pt = T.head;
	int js = 0, jt = 0;

	while (i < S.curlen && i < T.curlen) {
		i++;
		while (*(ps->ch + js) == ' ') {
			js++;
			if (js == CHUNKSIZE) {
				ps = ps->next;
				js = 0;
			}
		}

		while (*(pt->ch + jt) == ' ') {
			jt++;
			if (jt == CHUNKSIZE) {
				pt = pt->next;
				jt = 0;
			}
		}

		if (*(ps->ch + js) != *(pt->ch + jt))
			return *(ps->ch + js) - *(pt->ch + jt);
		else {
			js++;
			if (js == CHUNKSIZE) {
				ps = ps->next;
				js = 0;
			}
			jt++;
			if (jt == CHUNKSIZE) {
				pt = pt->next;
				jt = 0;
			}
		}
	}

	return S.curlen - T.curlen;
}

int concat_LSS(LSString * T, LSString S1, LSString S2)
{
	LSString a1, a2;

	init_LSS(&a1);
	init_LSS(&a2);
	copy_LSS(&a1, S1);
	copy_LSS(&a2, S2);
	T->curlen = S1.curlen + S2.curlen;
	T->head = a1.head;
	a1.tail->next = a2.head;
	T->tail = a2.tail;

	return 0;
}

int getSub_LSS(LSString * Sub, LSString S, int pos, int len)
{
	Chunk *p, *q;
	int i, k, n, flag = 1;

	if (pos < 1 || pos > S.curlen || len < 0 || len > S.curlen - pos + 1)
		return -1;

	n = len / CHUNKSIZE;
	if (len % CHUNKSIZE)
		n++;

	p = (Chunk *) malloc(sizeof(Chunk));
	(*Sub).head = p;

	for (i = 1; i < n; i++) {
		q = (Chunk *) malloc(sizeof(Chunk));
		p->next = q;
		p = q;
	}
	p->next = NULL;
	(*Sub).tail = p;
	(*Sub).curlen = len;
	for (i = len % CHUNKSIZE; i < CHUNKSIZE; i++)
		*(p->ch + i) = ' ';

	q = (*Sub).head;
	i = 0;
	p = S.head;
	n = 0;
	while (flag) {
		for (k = 0; k < CHUNKSIZE; k++)
			if (*(p->ch + k) != ' ') {
				n++;
				if (n >= pos && n <= pos + len - 1) {
					if (i == CHUNKSIZE) {
						q = q->next;
						i = 0;
					}
					*(q->ch + i) = *(p->ch + k);
					i++;
					if (n == pos + len - 1) {
						flag = 0;
						break;
					}
				}
			}
		p = p->next;
	}

	return 0;
}

int locationSub_LSS(LSString S, LSString T, int pos)
{
	int i, n, m;
	LSString sub;

	if (pos >= 1 && pos <= getLength_LSS(S)) {
		n = getLength_LSS(S);
		m = getLength_LSS(T);
		i = pos;
		while (i <= n - m + 1) {
			getSub_LSS(&sub, S, i, m);
			if (compare_LSS(sub, T) != 0)
				++i;
			else
				return i;
		}
	}

	return 0;
}

void zipString_LSS(LSString * S)
{
	int j, n = 0;
	Chunk *h = S->head;
	char *q;

	q = (char *)malloc((S->curlen + 1) * sizeof(char));
	while (h) {
		for (j = 0; j < CHUNKSIZE; j++)
			if (*(h->ch + j) != ' ') {
				*(q + n) = *(h->ch + j);
				n++;
			}
		h = h->next;
	}

	*(q + n) = 0;
	cleanContent_LSS(S);
	assign_LSS(S, q);
}

int insertSub_LSS(LSString * S, int pos, LSString T)
{
	int i, j, k;
	Chunk *p, *q;
	LSString t;

	if (pos < 1 || pos > getLength_LSS(*S) + 1)
		return -1;

	copy_LSS(&t, T);
	zipString_LSS(S);
	i = (pos - 1) / CHUNKSIZE;
	j = (pos - 1) % CHUNKSIZE;
	p = S->head;
	if (pos == 1) {
		t.tail->next = S->head;
		S->head = t.head;
	} else if (j == 0) {
		for (k = 1; k < i; k++)
			p = p->next;
		q = p->next;
		p->next = t.head;
		t.tail->next = q;
		if (q == NULL)
			S->tail = t.tail;
	} else {
		for (k = 1; k <= i; k++)
			p = p->next;

		q = (Chunk *) malloc(sizeof(Chunk));
		for (i = 0; i < j; i++)
			*(q->ch + i) = ' ';

		for (i = j; i < CHUNKSIZE; i++) {
			*(q->ch + i) = *(p->ch + i);
			*(p->ch + i) = ' ';
		}

		q->next = p->next;
		p->next = t.head;
		t.tail->next = q;
	}
	S->curlen += t.curlen;
	zipString_LSS(S);

	return 0;
}

int deleteSub_LSS(LSString * S, int pos, int len)
{
	int i = 1;
	Chunk *p = S->head;
	int j = 0;

	if (pos < 1 || pos > S->curlen - len + 1 || len < 0)
		return -1;

	while (i < pos) {
		while (*(p->ch + j) == ' ') {
			j++;
			if (j == CHUNKSIZE) {
				p = p->next;
				j = 0;
			}
		}

		i++;
		j++;
		if (j == CHUNKSIZE) {
			p = p->next;
			j = 0;
		}
	}

	while (i < pos + len) {
		while (*(p->ch + j) == ' ') {
			j++;
			if (j == CHUNKSIZE) {
				p = p->next;
				j = 0;
			}
		}

		*(p->ch + j) = ' ';
		i++;
		j++;
		if (j == CHUNKSIZE) {
			p = p->next;
			j = 0;
		}
	}

	S->curlen -= len;

	return 0;
}

int replaceSub_LSS(LSString * S, LSString T, LSString V)
{
	int i = 1;

	if (isEmpty_LSS(T))
		return -1;

	do {
		i = locationSub_LSS(*S, T, i);
		if (i) {
			deleteSub_LSS(S, i, getLength_LSS(T));
			insertSub_LSS(S, i, V);
			i += getLength_LSS(V);
		}
	} while (i);

	return 0;
}

void show_LSS(LSString T)
{
	int i = 0, j;
	Chunk *h;

	h = T.head;
	while (i < T.curlen) {
		for (j = 0; j < CHUNKSIZE; j++)
			if (*(h->ch + j) != ' ') {
				printf("%c", *(h->ch + j));
				i++;
			}
		h = h->next;
	}
}

int main()
{
	int pos, len;
	LSString t1, t2, t3, t4;
	char *s1 = "ABCDEFGHI", *s2 = "12345";
	char *s3 = "", *s4 = "asd#tr", *s5 = "ABCD";

	init_LSS(&t1);
	init_LSS(&t2);

	assign_LSS(&t1, s3);
	printf("\nString T1: ");
	show_LSS(t1);
	printf("\n\t%d[0-empty], len: %d", isEmpty_LSS(t1), getLength_LSS(t1));

	assign_LSS(&t1, s4);
	printf("\nString T1: ");
	show_LSS(t1);

	assign_LSS(&t1, s1);
	printf("\nString T1: ");
	show_LSS(t1);
	printf("\n\t%d[0-empty], len: %d", isEmpty_LSS(t1), getLength_LSS(t1));

	assign_LSS(&t2, s2);
	printf("\nString T2: ");
	show_LSS(t2);
	copy_LSS(&t3, t1);
	printf("\nCopy String T3 from String T1, then T3:\n\t");
	show_LSS(t3);

	init_LSS(&t4);
	assign_LSS(&t4, s5);
	printf("\nString T4: ");
	show_LSS(t4);
	replaceSub_LSS(&t3, t4, t2);
	printf("\nReplace String T4 in T3 to String T2, then T3:\n\t");
	show_LSS(t3);

	cleanContent_LSS(&t1);
	printf("\nClean T1, %d, len: %d", isEmpty_LSS(t1), getLength_LSS(t1));
	concat_LSS(&t1, t2, t3);
	printf("\nConcat String T2 and T3 to T1, then T1:\n\t");
	show_LSS(t1);

	zipString_LSS(&t1);
	printf("\nZip blank in String T1, then T1:\n\t");
	show_LSS(t1);

	pos = locationSub_LSS(t1, t3, 1);
	printf("\nLocation T3 in T1 Pos: %d", pos);
	insertSub_LSS(&t1, 3, t2);
	printf("\nInsert string T2 to String T1[3,*], then T1:\n\t");
	show_LSS(t1);

	getSub_LSS(&t2, t1, 3, 3);
	printf("\nGet sub String T2 from String T1[3,3], then T2:\n\t");
	show_LSS(t2);
	printf("\nCompare_LSS(t1,t2)=%d", compare_LSS(t1, t2));

	deleteSub_LSS(&t1, pos, len);
	printf("\nDelete string from String T1[3,3], then T1:\n\t");
	show_LSS(t1);
	printf("\n");

	return 0;
}
