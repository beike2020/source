###############################################################################
# This is a example of string handle. 
###############################################################################
.section .data
express:
	.asciz "  1: get a string and show\n  2: set a string and show\n  3: strcmp two strings\n  4: find a char from a string\n  5: get the string len\n  6: transform a string to upper\n\n"
eout:
	.asciz "Input error, please input value [1 - 6]!\n"
sstr0:
	.asciz "This is a test of the CMPS instructions\n"
sstr1:
	.ascii "This is a test of the CMPS instructions!\n"
sstr2:
	.ascii "r"
sstr3:
	.asciz "\n"
output1:
	.asciz "Find 'r' location: %d!\n"
output2:
	.asciz "The string length is %d!\n"
outputg:
    .asciz "The result is greater!\n"
outputl:
    .asciz "The result is less!\n"
len1:
	.int   41 
len2:
	.int   42 
divisor:
	.int   4

.section .bss
	.lcomm bufs, 	4
	.lcomm dstr,    41	

.section .text
.globl _start 
_start:
	nop  
	pushl 8(%esp) 
	call  atoi
	addl  $4,   %esp
	movl  %eax, bufs

	pushl $express
	call  printf
	addl  $4,   %esp

	cmp   $1,   bufs
	jne   func2 
	call  string_getc
	jmp   end
	
func2:
	cmp   $2,   bufs
	jne   func3 
	call  string_setc 
	jmp   end

func3:
	cmp   $3,   bufs
	jne   func4 
	call  string_cmp  
	jmp   end
	
func4:
	cmp   $4,   bufs
	jne   func5 
	call  string_chr 
	jmp   end

func5:
	cmp   $5,   bufs
	jne   func6 
	call  string_len 
	jmp   end

func6:
	cmp   $6,   bufs
	jne   errors 
	call  string_upper 
	jmp   end

errors:
	pushl $eout
	call  printf

end:	
	movl  $1,   %eax
	movl  $0,   %ebx
	int   $0x80

.type string_getc, @function
.globl string_getc
string_getc:
	pushl %ebp
	movl  %esp,		%ebp
	#jmp  reverse

	leal  sstr0,    %esi
	leal  dstr,     %edi
	movl  len1,     %ecx
	shrl  $2,       %ecx
	cld
	rep   movsl
	movl  len1,     %ecx
	andl  $3,       %ecx
	rep   movsb
	jmp   showr

reverse:
	nop
	leal  sstr0+39, %esi
	leal  dstr+39,  %edi
	movl  $40,      %ecx
	std
	rep   movsb
	jmp   showr

showr:
	movl  $4,       %eax
	movl  $1,       %ebx
	movl  $dstr,    %ecx
	movl  len1,     %edx
	int   $0x80
		
	movl  %ebp,     %esp
	popl  %ebp
	ret

.type string_setc, @function
.globl string_setc
string_setc:
	pushl %ebp
	movl  %esp,     %ebp

	leal  sstr2,    %esi
	leal  dstr,     %edi
	movl  len1,     %ecx
	cld
	lodsb
	rep   stosb

	movl  $4,       %eax
	movl  $1,       %ebx
	movl  $dstr,    %ecx
	movl  len1,     %edx
	int   $0x80
		
    pushl $sstr3
    call  printf
    addl  $4,       %esp

	movl  %ebp,     %esp
	popl  %ebp
	ret

.type string_cmp, @function
.globl string_cmp
string_cmp:
	pushl %ebp
	movl  %esp,     %ebp

	leal  sstr0,    %esi
	leal  sstr1,    %edi 
	movl  len1,     %ecx
	movl  len2,     %eax
	cmpl  %eax,     %ecx
	ja    longer
	xchg  %ecx,     %eax

longer:
	cld
	repe  cmpsb
	je    equal
	jg    greater
	
less:
    pushl $outputl
    call  printf
    addl  $4,       %esp
	jmp   retc 

greater:
    pushl $outputg
    call  printf
    addl  $4,       %esp
	jmp   retc 
	
equal:
	movl  len1,     %ecx
	movl  len2,     %eax
	cmpl  %ecx,     %eax
	jp    greater
	jl    less

retc:
	movl  %ebp,     %esp
	popl  %ebp
	ret

.type string_chr, @function
.globl string_chr
string_chr:
	pushl %ebp
	movl  %esp,     %ebp

	leal  sstr0,    %edi
	leal  sstr2,    %esi
	movl  len1,     %ecx
	lodsb
	cld
	repne scasb 
	jne   end 
	subw  len1,     %cx
	neg   %cx
	
showc:
    pushl %ecx
    pushl $output1
    call  printf
    addl  $8,       %esp

	movl  %ebp,     %esp
	popl  %ebp
	ret

.type string_len, @function
.globl string_len
string_len:
	pushl %ebp
	movl  %esp,     %ebp

	leal  sstr0,    %edi
	movl  $0xffff,  %ecx
	movb  $0,       %al
	cld
	repne scasb 
	jne   end
	subw  $0xffff,  %cx
	neg   %cx
	dec   %cx

showl:
    pushl %ecx
    pushl $output2
    call  printf
    addl  $8,       %esp

	movl  %ebp,   	%esp
	popl  %ebp
	ret

.type string_upper, @function
.globl string_upper
string_upper:
	pushl %ebp
	movl  %esp,   	%ebp

	leal  sstr0,   	%esi
	movl  %esi,   	%edi
	movl  len1,   	%ecx
	cld

loop1:
	lodsb
	cmpb  $'a',   	%al
	jl    skip
	cmpb  $'z',   	%al
	jg    skip
	subb  $0x20,  	%al
	
skip:
	stosb
	loop  loop1

showu:
	pushl $sstr0
	call  printf
	addl  $4,     	%esp

	movl  %ebp,   	%esp
	popl  %ebp
	ret

