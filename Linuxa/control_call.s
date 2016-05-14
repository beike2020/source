###############################################################
# This is a program test system contrl. 
###############################################################
.section .data 
express:
	.asciz " 1: test the simple jump operation\n 2: test the simple call operation\n 3: test the system sleep\n 4: check the parity flag bit\n 5: check the sign flag bit\n 6: check the overflow bit\n 7: test the simple get sum\n\n"
eout:
	.asciz "Input error, please input value [1 - 7]!\n"
output:
	.asciz "The value is %d\n"
outputh:
	.asciz "Hello,world!\n"
outputo:
	.asciz "Generage overflow value!\n"
result:
	.asciz "The sum is %d\n"
value:
	.int   21, 15, 34, 11, 6, 50, 32, 80, 10, 2

.section .bss
	.lcomm select, 4

.section .text
.globl _start 
_start:
	nop  
	pushl 8(%esp) 
	call  atoi
	addl  $4, 	%esp
	movl  %eax, select

	pushl $express
	call  printf
	addl  $4, 	%esp

	cmp   $1, 	select
	jne   func2 
	call  jump_test 
	jmp   end

func2:
	cmp   $2, 	select
	jne   func3 
	call  call_test 
	jmp   end

func3:
	cmp   $3, 	select
	jne   func4 
	call  call_sleep 
	jmp   end

func4:
	cmp   $4, 	select
	jne   func5 
	call  parityflag_check  
	jmp   end

func5:
	cmp   $5, 	select
	jne   func6 
	call  signflag_check  
	jmp   end

func6:
	cmp   $6, 	select
	jne   func7 
	call  overflow_check  
	jmp   end

func7:
	cmp   $7, 	select
	jne   errors 
	call  get_sum 
	jmp   end

errors:
	pushl $eout
	call  printf

end:
	movl  $1, 	%eax
	movl  $0, 	%ebx
	int   $0x80

.type jump_test, @function
.globl jump_test
jump_test:
	pushl %ebp
	movl  %esp, %ebp

	movl  $1,   %eax
	jmp   overhere
	movl  $10,  %ebx
	jmp   show

overhere:
	movl  $20,  %ebx

show:
	pushl %ebx
	pushl $output	
	call  printf	
	add   $8,   %esp

	movl  %ebp, %esp
	popl  %ebp
	ret

.type call_test, @function
.globl call_test 
call_test:
	pushl %ebp
	movl  %esp, %ebp

	movl  $15, 	%eax
	movl  $10, 	%ebx
	cmp   %eax, %ebx
	jge   greater
	pushl $0
	pushl $output
	call  printf
	add   $8, 	%esp
	
	movl  %ebp, %esp
	popl  %ebp
	ret

greater:
	pushl $1
	pushl $output
	call  printf
	add   $8, 	%esp
	
	movl  %ebp, %esp
	popl  %ebp
	ret

.type call_sleep, @function
.globl call_sleep 
call_sleep:
	pushl %ebp
	movl  %esp, %ebp

	movl  $3, 	%ecx
loop1:
	pushl %ecx
	pushl $outputh
	call  printf
	addl  $4,  	%esp
	pushl $3
	call  sleep
	addl  $4,  	%esp
	popl  %ecx
	loop  loop1

	movl  %ebp, %esp
	popl  %ebp
	ret

.type parityflag_check, @function
.globl parityflag_check
parityflag_check:
	pushl %ebp
	movl  %esp, %ebp

	movl  $5, 	%ebx
	subl  $3, 	%ebx
	jp    there
	pushl %ebx
	pushl $output
	call  printf
	add   $8, 	%esp
	
there:
	movl  %ebp, %esp
	popl  %ebp
	ret

.type signflag_check, @function
.globl signflag_check
signflag_check:
	pushl %ebp
	movl  %esp, %ebp

	movl  $9, 	%edi
loops:
	pushl value(, %edi, 4)
	pushl $output
	call  printf
	addl  $8, 	%esp
	dec   %edi
	jns   loops

	movl  %ebp, %esp
	popl  %ebp
	ret

.type overflow_check, @function
.globl overflow_check
overflow_check:
	pushl %ebp
	movl  %esp, %ebp

	movl  $0xffffffff, %eax
	inc   %eax
	jc    overflow

	movl  $0xffffffff, %ebx
	addl  $1, 	%ebx
	jc    overflow

overflow:
	pushl $outputo
	call  printf
	addl  $4, 	%esp

	movl  %ebp, %esp
	popl  %ebp
	ret

.type get_sum, @function
.globl get_sum 
get_sum:
	pushl %ebp
	movl  %esp, %ebp

	movl  $100, %ecx
	movl  $0,   %eax
	jcxz  showsum 

accumulate:
	addl  %ecx, %eax
	loop  accumulate

showsum:
	pushl %eax
	pushl $result	
	call  printf	
	add   $8, 	%esp

	movl  %ebp, %esp
    popl  %ebp
    ret
