/**************************************************************
* An example of using c variables. 
**************************************************************/
#include <stdio.h>

#define GREATER(a, b, ret) ({ \
	asm ("cmp  %1, %2\n\t" \
		 "jge  0f\n\t" \
		 "movl %1, %0\n\t" \
		 "jmp  1f\n" \
		 "0:\n\t" \
		 "movl %2, %0\n" \
		 "1:" \
		 : "=r"(ret) \
		 : "r"(a), "r"(b)); }) 

int a = 10;
int b = 20;
int result;

int global_test()
{
	__asm__ ( "pusha\n\t"
		  	  "movl   a,    %eax\n\t"
		      "movl   b,    %ebx\n\t"
		      "imull  %ebx, %eax\n\t"
	   	      "movl   %eax, result\n\t"
		      "popa");
	printf("The answer is %d\n", result);

	return 0;
}

int local_test()
{
	int ret;
	int data1 = 10;
	int data2 = 20;

	asm ( "imull  %%edx, %%ecx\n\t"
	   	  "movl   %%ecx, %%eax\n\t"
		  : "=a"(ret)
		  : "d"(data1), "c"(data2));
	printf("The answer is %d\n", ret);

	return 0;
}

int default_output()
{
	char input[16] = {"Hello, world!\n"};
	char output[16];
	int  length = 15;

	asm volatile ("cld\n\t"
				  "rep movsb"
				  :
				  : "S"(input), "D"(output), "c"(length));
	printf("%s", output);

	return 0;
}

int placeholder_mutiple()
{
	int ret;
	int data1 = 10;
	int data2 = 20;

	asm ("imull %1, %2\n\t"
		 "movl  %2, %0"
	 	 : "=r"(ret)
		 : "r"(data1), "r"(data2));
	printf("The result is %d\n", ret);

	return 0;
}

int placeholder_common() 
{
	int data1 = 10;
	int data2 = 20;

	asm ("imull %1, %0\n\t"
	 	 : "=r"(data2)
		 : "r"(data1), "0"(data2));
	printf("The result is %d\n", data2);

	return 0;
}

int placeholder_replace() 
{
	int data1 = 10;
	int data2 = 20;

	asm ("imull %[value1], %[value2]\n\t"
	 	 : [value2] "=r"(data2)
		 : [value1] "r"(data1), "0"(data2));
	printf("The result is %d\n", data2);

	return 0;
}

int register_change()
{
	int ret = 20;
	int data1 = 10;
	
	asm ("movl %1,    %%eax\n\t"
		 "addl %%eax, %0"
		 : "=r"(ret)
		 : "r"(data1), "0"(ret)
		 : "%eax");
	printf("The result is %d\n", ret);

	return 0;
}

int memory_handle()
{
	int ret;
	int divisor = 5;
	int dividend = 20;
	
	asm ("divb %2\n\t"
		 "movl %%eax, %0"
		 : "=m"(ret)
		 : "a"(dividend), "m"(divisor));
	printf("The result is %d\n", ret);

	return 0;
}

int fpu_default()
{
	float angle = 90;
	float radian, cosine, sine;
	
	radian = angle / 180 * 3.14159;
	asm ("fsincos"
		 : "=t"(cosine), "=u"(sine)
		 : "0"(radian));
	printf("The cosine is %f, and the sine is %f\n", cosine, sine);

	return 0;
}

int fpu_special()
{
	float area;
	int radius = 10;
	
	asm ("fild  %1\n\t"
	     "fimul %1\n\t"
		 "fldpi\n\t"
		 "fmul %%st(1), %%st(0)"
		 : "=t"(area)
		 : "m"(radius)
		 : "%st(1)");
	printf("The result is %f\n", area);

	return 0;
} 

int jmp_handle()
{
	int ret;
	int a = 10;
	int b = 20;
	
	asm ("cmp  %1, %2\n\t"
	     "jge  greater\n\t"
		 "movl %1, %0\n\t"
		 "jmp  end\n"
		 "greater:\n\t"
		 "movl %2, %0\n"
		 "end:"
		 : "=r"(ret)
		 : "r"(a), "r"(b));
	printf("The result is %d\n", result);

	return 0;
}

int jmp_common()
{
	int ret;
	int a = 10;
	int b = 20;

	asm ("cmp  %1, %2\n\t"
		 "jge  0f\n\t"
		 "movl %1, %0\n\t"
		 "jmp  1f\n"
		 "0:\n\t"
		 "movl %2, %0\n"
		 "1:"
		 : "=r"(ret)
		 : "r"(a), "r"(b));
	printf("The larger value is %d\n", ret);

	return 0;
}

int macros_handle()
{
	int ret;
	int data1 = 10;
	int data2 = 20;
	
	GREATER(data1, data2, ret);
	printf("a = %d, b = %d, result: %d\n", data1, data2, ret);
	data1 = 30;
	GREATER(data1, data2, ret);
	printf("a = %d, b = %d, result: %d\n", data1, data2, ret);

	return 0;
}

int main()
{
	global_test();
	local_test();
	default_output();
	placeholder_mutiple();
	placeholder_common();
	placeholder_replace();
	register_change();
	memory_handle();
	fpu_default();
	fpu_special();
	jmp_handle();
	jmp_common();
	macros_handle();

	return 0;
}
