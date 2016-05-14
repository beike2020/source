/**********************************************************
 * Function:	A example of string sort method.
 * Author:	forwarding2012@yahoo.com.cn			  		 
 * Date:		2012.01.01				  		  
 * Complie: 	gcc -Wall -g Sort_method.c -o Sort_method
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int check_model()
{
	int choices;

	printf("Test program as follow:\n");
	printf(" 1: test the method of bubble sort\n");
	printf(" 2: test the method of select sort\n");
	printf(" 3: test the method of insert sort\n");
	printf(" 4: test the method of shell sort\n");
	printf(" 5: test the method of quick sort\n");
	printf(" 6: test the method of heap sort\n");
	printf("Please input test type: ");
	scanf("%d", &choices);

	return choices;
}

void bubble_sort(int *x, int n)
{
	int i, j, tmp;

	for (i = 0; i < n - 1; i++) {
		for (j = i + 1; j < n; j++) {
			if (x[j] < x[i]) {
				tmp = x[i];
				x[i] = x[j];
				x[j] = tmp;
			}
		}
	}
}

void select_sort(int *x, int n)
{
	int i, j, min, tmp;

	for (i = 0; i < n - 1; i++) {
		min = i;
		for (j = i + 1; j < n; j++) {
			if (x[j] < x[min])
				min = j;
		}

		if (min != i) {
			tmp = x[i];
			x[i] = x[min];
			x[min] = tmp;
		}
	}
}

void insert_sort(int *x, int n)
{
	int i, j, tmp;

	for (i = 1; i < n; i++) {
		tmp = x[i];
		for (j = i - 1; (j >= 0 && tmp < x[j]); j--)
			x[j + 1] = x[j];
		x[j + 1] = tmp;
	}
}

void shell_sort(int *x, int n)
{
	int i, j, k, tmp;

	for (i = n / 2; i > 0; i = i / 2) {
		for (j = i; j < n; j++) {
			tmp = x[j];
			for (k = j - i; (k >= 0 && tmp < x[k]); k -= i)
				x[k + i] = x[k];
			x[k + i] = tmp;
		}
	}
}

void quick_sort(int *x, int left, int right)
{
	int i, j, temp;

	if (left < right) {
		i = left;
		j = right;
		temp = x[i];

		while (i < j) {
			while (i < j && x[j] > temp)
				j--;

			if (i < j)
				x[i++] = x[j];

			while (i < j && x[i] < temp)
				i++;

			if (i < j)
				x[j--] = x[i];
		}

		x[i] = temp;
		quick_sort(x, left, i - 1);
		quick_sort(x, i + 1, right);
	}
}

void sift_func(int *x, int n, int s)
{
	int i, j, tmp;

	i = s;
	j = 2 * i + 1;
	tmp = x[i];

	while (j < n) {
		if (j < n - 1 && x[j] < x[j + 1])
			j++;

		if (tmp >= x[j])
			break;

		x[i] = x[j];
		i = j;
		j = 2 * i + 1;
	}

	x[i] = tmp;
}

void heap_sort(int *x, int n)
{
	int i, j;
	int tmp;

	for (i = n / 2 - 1; i >= 0; i--)
		sift_func(x, n, i);

	for (j = n - 1; j >= 1; j--) {
		tmp = x[0];
		x[0] = x[j];
		x[j] = tmp;
		sift_func(x, j, 0);
	}
}

int main(int argc, char *argv[])
{
	int a[10] = { 6, 23, 22, 9, 1, 67, 15, 20, 9, 3 };
	int i = 0, choice = 0;
	int *p = a;

	if (argc < 2)
		choice = check_model();

	switch (choice) {
		//test the method of bubble sort
	case 1:
		bubble_sort(p, 10);
		break;

		//test the method of select sort
	case 2:
		select_sort(p, 10);
		break;

		//test the method of insert sort
	case 3:
		insert_sort(p, 10);
		break;

		//test the method of shell sort
	case 4:
		shell_sort(p, 10);
		break;

		//test the method of quick sort
	case 5:
		quick_sort(p, 0, 9);
		break;

		//test the method of heap sort
	case 6:
		heap_sort(p, 10);
		break;

		//default do nothing
	default:
		break;
	}

	p = a;
	for (i = 0; i < 10; i++)
		printf("%d ", *p++);
	printf("\n");

	return 0;
}
