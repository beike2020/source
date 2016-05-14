###############################################################################
# An example of base math instruction. 
###############################################################################
.section .data
express:
	.asciz " 1: add two normal numbers\n 2: add two big numbers\n 3: sub two normal numbers\n 4: sub two big numbers\n 5: mull two normal numbers\n 6: div two normal numbers\n 7: mov bit a number\n 8: operation a unpackage BSD\n 9: operation a package BSD\n\n"
eout:
	.asciz "Input error, please input value [1 - 9]!\n"
carryout:
	.asciz "The result generate carry bit!\n"
overflowout:
	.asciz "The result generate overflow bit!\n"
result:
	.asciz "The result is %d!\n"
output:
	.asciz "The largest value is %d!\n"
output1:
	.asciz "The sum result is %qd\n"
output2:
	.asciz "The sub result is %qd\n"
output3:
	.asciz "The sub result is %qd\n"
divoutput:
	.asciz "The quotient is %d, and the remainder is %d\n"
divisor1:
	.int 25
divisor2:
	.int -25
data:
	.int 10
data1:
	.quad 7252051615
data2:
	.quad 5732348928
dividend:
	.quad 8335
muldata:
	.quad 0
value1:
	.byte 0x05, 0x02, 0x01, 0x08, 0x02
value2:
	.byte 0x03, 0x03, 0x09, 0x02, 0x05

.section .rodata
	.equ COUNT, 10

.section .bss
	.lcomm bufs, 4
	.lcomm bufb, 6

.section .text
.globl _start 
_start:
	nop  
	pushl 8(%esp) 
	call  atoi
	addl  $4, 	%esp
	movl  %eax, bufs

	pushl $express
	call  printf
	addl  $4, 	%esp

	cmp   $1, 	bufs
	jne   func2 
	call  add_normal 
	jmp   end
	
func2:
	cmp   $2, 	bufs
	jne   func3 
	call  add_bignum 
	jmp   end

func3:
	cmp   $3, 	bufs
	jne   func4 
	call  sub_normal  
	jmp   end
	
func4:
	cmp   $4, 	bufs
	jne   func5 
	call  sub_bignum 
	jmp   end

func5:
	cmp   $5, 	bufs
	jne   func6 
	call  mul_normal 
	jmp   end

func6:
	cmp   $6, 	bufs
	jne   func7 
	call  div_normal 
	jmp   end

func7:
	cmp   $7, 	bufs
	jne   func8 
	call  sal_normal 
	jmp   end

func8:
	cmp   $8, 	bufs
	jne   func9 
	call  bsd_nopackage 
	jmp   end

func9:
	cmp   $9, 	bufs
	jne   errors 
	call  bsd_package 
	jmp   end

errors:
	pushl $eout
	call  printf

end:	
	movl  $1, 	%eax
	movl  $0, 	%ebx
	int   $0x80

.type  add_normal, @function
.globl add_normal 
add_normal:
	pushl %ebp
	movl  %esp, %ebp

	movl  $0,   %eax
	movb  $20,  %al
	addb  $10,  %al
	movsx %al,  %eax
	addl  data, %eax
	pushl %eax
	pushl $result
	call  printf
	addl  $8,   %esp

	movl  $-10, %eax
	movl  $-200, %ebx
	movl  $80,  %ecx
	addl  $-40, %eax
	addl  %ecx, %eax
	addl  %ebx, %eax
	pushl %eax
	pushl $result
	call  printf
	addl  $8,   %esp

	movl  $0,   %eax
	movl  $0,   %ebx
	movb  $190, %bl
	movb  $100, %al
	addb  %al,  %bl
	jnc   nbit

carrybit:	
	pushl %ebx
	pushl $result
	call  printf
	addl  $8,   %esp

	movl  $-1590876934, %ebx
	movl  $-1259230143, %eax
	addl  %eax, %ebx
	jo    overflow
	jmp   nbit

overflow:
	pushl %eax
	pushl $result
	call  printf
	addl  $8,   %esp
	
nbit:
	movl  %ebp, %esp
	popl  %ebp
	ret

.type  add_bignum, @function
.globl add_bignum 
add_bignum:
	pushl %ebp
	movl  %esp,    %ebp

	movl  data1,   %ebx
	movl  data1+4, %eax
	movl  data2,   %edx
	movl  data2+4, %ecx
	addl  %ebx,    %edx
	adcl  %eax,    %ecx
	pushl %ecx
	pushl %edx
	pushl $output1
	call  printf
	addl  $12,     %esp

	subl  %ebx,    %edx
	subl  %eax,    %ecx
	pushl %ecx
	pushl %edx
	pushl $output2
	call  printf
	addl  $12,     %esp
	
	movl  %ebp,    %esp
	popl  %ebp
	ret

.type  sub_normal, @function
.globl sub_normal 
sub_normal:
	pushl %ebp
	movl  %esp, %ebp

	movl  $0,   %eax
	movl  $0,   %ebx
	movl  $0,   %ecx
	movb  $20,  %al
	subb  $10,  %al
	movsx %al,  %eax
	movw  $100, %cx
	subw  %cx,  %bx
	movsx %bx,  %ebx
	movl  $100, %edx
	subl  %eax, %edx
	subl  data, %eax
	subl  %eax, data
	pushl data
	pushl $result
	call  printf
	addl  $8,   %esp

test_scarry:
	subl  $100, data
	jnc   test_soverflow
	pushl $carryout
	call  printf
	addl  $4,   %esp

test_soverflow:
	movl  $-1590876934, %ebx
	movl  $1259230143,  %eax
	subl  %eax, %ebx
	jno   rets
	pushl $overflowout
	call  printf
	addl  $4,   %esp

rets:
	movl  %ebp, %esp
	popl  %ebp
	ret

.type  sub_bignum, @function
.globl sub_bignum 
sub_bignum:
	pushl %ebp
	movl  %esp,    %ebp

	movl  data1,   %ebx
	movl  data1+4, %eax
	movl  data2,   %edx
	movl  data2+4, %ecx
	subl  %ebx,    %edx
	sbbl  %eax,    %ecx
	pushl %ecx
	pushl %edx
	pushl $output1
	call  printf
	addl  $12,     %esp		

	movl  %ebp,    %esp
	popl  %ebp
	ret

.type  mul_normal, @function
.globl mul_normal 
mul_normal:
	pushl %ebp
	movl  %esp,    %ebp

mulbig:
	movl  $315814, %eax
	movl  $165432, %ebx
	mull  %ebx
	movl  %eax,    muldata
	movl  %edx,    muldata+4
	pushl %edx
	pushl %eax
	pushl $output3
	call  printf
	addl  $12,     %esp		

mulbit:
	movl  $0,      %eax
	movl  $10,     %ebx
	movl  $-35,    %ecx
	imull %ebx,    %ecx	
	movl  $400,    %edx
	imull $2,      %edx, %eax
	pushl %ecx
	pushl $result
	call  printf
	addl  $8,      %esp	
	pushl %eax
	pushl $result
	call  printf
	addl  $8,      %esp

muloverflow:
	movl  $0,      %eax
	movl  $0,      %ecx
	movw  $680,    %ax
	movw  $100,    %cx
	imulw %cx
	jno   retm
	pushl $overflowout
	call  printf
	addl  $4,      %esp

retm:
	movl  %ebp,    %esp
	popl  %ebp
	ret

.type  div_normal, @function
.globl div_normal 
div_normal:
	pushl %ebp
	movl  %esp,       %ebp

unsigneddiv:
	movl  dividend,   %eax
	movl  dividend+4, %edx
	divl  divisor1
	pushl %eax
	pushl %edx
	pushl $divoutput
	call  printf
	add   $12,        %esp

signeddiv:
	movl  dividend,   %eax
	movl  dividend+4, %edx
	idivl divisor2
	pushl %eax
	pushl %edx
	pushl $divoutput
	call  printf
	add   $12,        %esp

	movl  %ebp,       %esp
	popl  %ebp
	ret

.type  sal_normal, @function
.globl sal_normal 
sal_normal:
	pushl %ebp
	movl  %esp, %ebp
	
	movl  $10,  %ebx
	sall  %ebx
	movb  $2,   %cl
	sall  %cl,  %ebx
	sall  $2,   %ebx
	pushl %ebx
	pushl $result
	call  printf
	add   $8,   %esp

	movl  %ebp, %esp
	popl  %ebp
	ret

.type  bsd_nopackage, @function
.globl bsd_nopackage 
bsd_nopackage:
	pushl %ebp
	movl  %esp, %ebp

	xor   %edi, %edi
	movl  $5,   %ecx
	clc

loopbn:
	movb  value1(, %edi, 1), %al
	adcb  value2(, %edi, 1), %al
	aaa
	movb  %al,  bufb(, %edi, 1)
	inc   %edi
	loop  loopbn
	adcb  $0,   bufb(, %edi, 4)

	movl  %ebp, %esp
	popl  %ebp
	ret

.type  bsd_package, @function
.globl bsd_package 
bsd_package:
	pushl %ebp
	movl  %esp, %ebp

	xor   %edi, %edi
	movl  $5,   %ecx
	clc

loopbp:
	movb  value1(, %edi, 1), %al
	sbbb  value2(, %edi, 1), %al
	das
	movb  %al,  bufb(, %edi, 1)
	inc   %edi
	loop  loopbp
	sbbb  $0,   bufb(, %edi, 4)

	movl  %ebp, %esp
	popl  %ebp
	ret
