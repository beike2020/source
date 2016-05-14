###############################################################
# Find the max value from set. 
###############################################################
.section .data
express:
	.asciz " 1: find the largest number\n 2: bubble a set number\n 3: swap a large number\n 4: change a 4 bits number\n 5: change a 8 bits number\n 6: test the stack operation\n\n"
input:
	.asciz "The %dth num is %d\n"
output:
	.asciz "The largest value is %d!\n"
result:
	.asciz "The result is %d!\n"
eout:
	.asciz "Input error, please input value [1 - 6]!\n"
values:
	.int   105, 235, 61, 315, 134, 221, 53, 145, 117, 5
matchs:
	.byte  0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88
data:
	.int   10

.section .rodata
	.equ   COUNT, 10

.section .bss
	.lcomm bufs, 4

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
	addl  $4,   %esp

	cmp   $1,   bufs
	jne   func2 
	call  findmax 
	jmp   end
	
func2:
	cmp   $2,   bufs
	jne   func3 
	call  bubble 
	jmp   end

func3:
	cmp   $3,   bufs
	jne   func4 
	call  swaptest  
	jmp   end
	
func4:
	cmp   $4,   bufs
	jne   func5 
	call  cchg4b 
	jmp   end

func5:
	cmp   $5,   bufs
	jne   func6 
	call  cchg8b 
	jmp   end

func6:
	cmp   $6,   bufs
	jne   errors 
	call  stack_io 
	jmp   end

errors:
	pushl $eout
	call  printf

end:	
	movl  $1,   %eax
	movl  $0,   %ebx
	int   $0x80

.type findmax, @function
.globl findmax
findmax:
	pushl %ebp
	movl  %esp,		%ebp

	movl  $0,     	%edi
loopr:
	movl  values(, %edi, 4), %eax
	pushl %eax
	pushl %edi	
	pushl $input
	call  printf
	addl  $12,    	%esp
	inc   %edi
	cmpl  $COUNT, 	%edi 
	jb    loopr

intc:
	movl  values, 	%ebx
	movl  $1,     	%edi
loopc:
	movl  values(, %edi, 4), %eax
	cmp   %ebx,   	%eax
	cmova %eax,   	%ebx
	inc   %edi
	cmp   $COUNT, 	%edi
	jne   loopc
	pushl %ebx
	pushl $output
	call  printf
	
	movl  %ebp,   	%esp
	popl  %ebp
	ret

.type bubble, @function
.globl bubble
bubble:
	pushl %ebp
	movl  %esp,    	%ebp

	movl  $values, 	%esi
	movl  $9,      	%ecx
	movl  $9,      	%ebx

loops:
	movl  (%esi),  	%eax
	cmp   %eax,    	4(%esi)
	jge   skip
	xchg  %eax,    	4(%esi)
	movl  %eax,    	(%esi)
	
skip:
	add   $4,      	%esi
	dec   %ebx
	jnz   loops
	dec   %ecx
	jz    show 
	movl  $values, 	%esi
	movl  %ecx,    	%ebx
	jmp   loops

show:
	movl  $0,      	%edi

loopp:
	movl  values(, %edi, 4), %eax
	pushl %eax
	pushl $result
	call  printf
	addl  $8,      	%esp
	inc   %edi
	cmpl  $10,     	%edi
	jne   loopp

	movl  %ebp,    	%esp
	popl  %ebp
	ret

.type swaptest, @function
.globl swaptest
swaptest:
	pushl %ebp
	movl  %esp,   	%ebp

	movl  $0x12345678, %ebx
	bswap %ebx
	pushl %ebx
	pushl $result
	call  printf
	addl  $8,     	%esp
	
	movl  %ebp,   	%esp
	popl  %ebp
	ret

.type cchg4b, @function
.globl cchg4b 
cchg4b:
	pushl %ebp
	movl  %esp,   	%ebp

	movl  $10,    	%eax
	movl  $5,     	%ebx
	cmpxchg %ebx, 	data 
	pushl data
	pushl $result
	call  printf
	addl  $8,     	%esp
	
	movl  %ebp,   	%esp
	popl  %ebp
	ret

.type cchg8b, @function
.globl cchg8b 
cchg8b:
	pushl %ebp
	movl  %esp,   	%ebp

	movl  $0x44332211, %eax
	movl  $0x88776655, %edx
	movl  $0x11111111, %ebx
	movl  $0x22222222, %ecx
	cmpxchg8b matchs 
	pushl matchs 
	pushl $result
	call  printf
	addl  $8,     	%esp
	
	movl  %ebp,   	%esp
	popl  %ebp
	ret

.type stack_io, @function
.globl stack_io 
stack_io:
	pushl %ebp
	movl  %esp,   	%ebp

	movl  $24420, 	%ecx
	movw  $350,   	%bx
	movl  $100,   	%eax
	pushl %ecx
	pushw %bx
	pushl %eax
	pushl data
	pushl $data

	popl  %eax
	popl  %eax
	popl  %eax
	popw  %ax
	popl  %eax
	pushl %eax
	pushl $result
	call  printf
	addl  $8,     	%esp

	movl  %ebp,   	%esp
	popl  %ebp
	ret
