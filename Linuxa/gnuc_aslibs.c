#include <stdio.h>

int output_func(void);
int square_func(int);
float area_func(int);
char *cpuid_func(void);
int greater_func(int, int); 
float fpmatch_func(float, int, float, float, float, int, float, float);

int print_text()
{
	printf("This is a test.\n");
	output_func();
	printf("Now for the second time.\n");
	output_func();
	printf("This completes the test.\n");

	return 0;
}

int io_value()
{
	int i = 2;
	int j = 10;
	
	printf("The square_func of %d is %d\n", i, square_func(i));
	printf("The square_func of %d is %d\n", j, square_func(j));
	
	return 0;
}

int output_string()
{
	printf("The CPUID is '%s'\n", cpuid_func());

	return 0;
}

int computer_area()
{
	float ret;
	int radius = 10;
	
	ret = area_func(radius);
	printf("The result is %f\n", ret);
	
	ret = area_func(2);
	printf("The result is %f\n", ret);

	return 0;
}

int get_greater()
{
	int i = 10;
	int j = 20;
	
	printf("The larger value is %d\n", greater_func(i, j));
	
	return 0;
}

int input_mutiple()
{
	float value1 = 43.65;
	int   value2 = 22;
	float value3 = 76.34;
	float value4 = 3.1;
	float value5 = 12.43;
	int   value6 = 6;
	float value7 = 140.2;
	float value8 = 94.21;
	float ret;

	ret = fpmatch_func(value1, value2, value3, value4, value5, 
					   value6, value7, value8);
	printf("The final result is %f\n", ret);
	
	return 0;
}

int main()
{
	print_text();
	io_value();
	output_string();
	computer_area();
	get_greater();
	input_mutiple();

	return 0;	
}
