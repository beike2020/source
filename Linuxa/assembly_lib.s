.section .data 
str1:
	.ascii "This is a test message from the asm function\n"
size1:
	.int   45

.section .bss
	.comm  output,	13


.section .text
.type output_func, @function
.globl output_func
output_func:
	pushl %ebp
	movl  %esp, 	%ebp
	pushl %ebx

	movl  $4, 		%eax
	movl  $1, 		%ebx
	movl  $str1, 	%ecx
	movl  size1, 	%edx
	int   $0x80
	
	popl  %ebx
	movl  %ebp, 	%esp
	popl  %ebp
	ret

.type square_func, @function
.globl square_func
square_func:
	pushl %ebp
	movl  %esp,		%ebp

	movl  8(%ebp),  %eax
	imull %eax,		%eax

	movl  %ebp,		%esp
	popl  %ebp
	ret

.type cpuid_func, @function
.globl cpuid_func
cpuid_func:
	pushl %ebp
	movl  %esp,		%ebp

	pushl %ebx
	movl  $0,		%eax
	cpuid
	movl  $output,	%edi
	movl  %ebx,		(%edi)
	movl  %edx,		4(%edi)
	movl  %ecx,		8(%edi)
	movl  $output,	%eax
	popl  %ebx

	movl  %ebp,		%esp
	popl  %ebp
	ret

.type area_func, @function
.globl area_func
area_func:
	pushl %ebp
	movl  %esp,		%ebp

	fldpi
	filds 8(%ebp)
	fmul  %st(0),	%st(0)
	fmul  %st(1),	%st(0)

	movl  %ebp,		%esp
	popl  %ebp
	ret

.type greater_func, @function
.globl greater_func
greater_func:
	pushl %ebp
	movl  %esp,		%ebp

	movl  8(%ebp),	%eax
	movl  12(%ebp),	%ecx
	cmpl  %ecx,		%eax
	jge   endg
	movl  %ecx,		%eax

endg:
	movl  %ebp,		%esp
	popl  %ebp
	ret

.type  fpmatch_func, @function
.globl fpmatch_func 
fpmatch_func:
	pushl %ebp
	movl  %esp,    	%ebp

	finit
	flds  8(%ebp)
	fidiv 12(%ebp)
	flds  16(%ebp)
	flds  20(%ebp)
	fmul  %st(1),  	%st(0)
	fadd  %st(2),  	%st(0)
	flds  24(%ebp)
	fimul 28(%ebp)
	flds  32(%ebp)
	flds  36(%ebp)
	fdivrp
	fsubr %st(1),  	%st(0)
	fdivr %st(2),  	%st(0)

	movl  %ebp,    	%esp
	popl  %ebp
	ret
