/*****************************************************************************
 * Function:	A example of search method.
 * Author: 	forwarding2012@yahoo.com.cn			  		 
 * Date:		2012.01.01				  		  
 * Complie: 	gcc -Wall -g Search_method.c -o Search_method
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

void show_Data(int *t, int length)
{
	int i;

	for (i = 0; i < length; i++)
		printf("A[%d]: %d\n", i, t[i]);
}

void sort_Data(int *t, int length)
{
	int tmp;
	int i, j;

	for (i = 0; i < length; i++) {
		for (j = i + 1; j < length; j++) {
			if (t[i] > t[j]) {
				tmp = t[i];
				t[i] = t[j];
				t[j] = tmp;
			}
		}
	}
}

int search_Data_byOrder(int *t, int k, int length)
{
	int i;

	for (i = length; i >= 0; i--) {
		if (t[i] == k)
			return i;
	}

	return -1;
}

int search_Data_byBin(int *t, int k, int length)
{
	int low = 0, mid = 0;
	int high = length - 1;

	while (low <= high) {
		mid = (low + high) / 2;
		if (k == t[mid])
			return mid;
		else if (k < t[mid])
			high = mid - 1;
		else
			low = mid + 1;
	}

	return -1;
}

int search_Data_byFibonacci(int *a, int key, int n)
{
	int i, low = 0, mid = 0, high = n - 1;
	int F[10], k = 0;

	F[0] = F[1] = 1;
	for (i = 2; i < 10; ++i)
		F[i] = F[i - 2] + F[i - 1];

	while (n > F[k] - 1)
		++k;

	for (i = n; i < F[k] - 1; ++i)
		a[i] = a[high];

	while (low <= high) {
		mid = low + F[k - 1] - 1;
		if (a[mid] > key) {
			high = mid - 1;
			k = k - 1;
		} else if (a[mid] < key) {
			low = mid + 1;
			k = k - 2;
		} else {
			if (mid <= high)
				return mid;
			else
				return -1;
		}
	}

	return -1;
}

int main()
{
	int *t = NULL;
	int key = 0, index = 0;
	int a[10] = { 6, 23, 22, 9, 1, 67, 15, 20, 9, 3 };

	printf("\nInit data start: ");
	if ((t = (int *)malloc(sizeof(int) * 10)) == NULL)
		return -1;

	for (index = 0; index < 10; index++)
		t[index] = a[index];

	printf("\nData as follow:\n");
	show_Data(t, 10);

	printf("\nAfter sort all data:\n");
	sort_Data(t, 10);
	show_Data(t, 10);

	printf("\nplease input you search data:");
	scanf("%d", &key);
	if ((index = search_Data_byOrder(t, key, 10)) >= 0)
		printf("\nUse Order-search find: A[%d]", index);
	else
		printf("\nCan't find it by Order-Search method!");

	if ((index = search_Data_byBin(t, key, 10)) >= 0)
		printf("\nUse Bin-Search find: A[%d]", index);
	else
		printf("\nCan't find it by Bin-Search method!");

	if ((index = search_Data_byFibonacci(t, key, 10)) >= 0)
		printf("\nUse Fibonacci-Search find: A[%d]\n", index);
	else
		printf("\nCan't find it by Fibonacci-Search method!\n");

	return 0;
}
