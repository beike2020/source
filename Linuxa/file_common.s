###############################################################
# A example of writing or reading data from a file. 
###############################################################
.section .data 
express:
	.asciz "  1: test write a file\n  2: test read a file\n  3: test nmap a file\n\n"
eout:
	.asciz "Input error, please input value [1 - 3]!\n"
fname:
	.asciz "/tmp/cpuid.txt"
output:
	.ascii "The processor Vendor ID is 'XXXXXXXXXXXX'\n"

.section .bss
	.lcomm fd,     4
	.lcomm size,   4
	.lcomm mapfd,  4
	.lcomm select, 4
	.lcomm buffer, 42

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
	call  write_file 
	jmp   end

func2:
	cmp   $2, 	select
	jne   func3 
	call  read_file
	jmp   end

func3:
	cmp   $3, 	select
	jne   errors 
	call  mmap_file  
	jmp   end

errors:
	pushl $eout
	call  printf

end:
	movl  $1, 	%eax
	movl  $0, 	%ebx
	int   $0x80

.type write_file, @function
.globl write_file
write_file:
	pushl %ebp
	movl  %esp, 	%ebp

	movl  $0,      	%eax
	cpuid
	movl  $output, 	%edi
	movl  %ebx,    	28(%edi)
	movl  %edx,    	32(%edi)
	movl  %ecx,    	36(%edi)
	jmp   aopen 
	
wopen:
	movl  $5,      	%eax
	movl  $fname, 	%ebx
	movl  $01101,  	%ecx 
	movl  $0644,   	%edx	
	int   $0x80
	test  %eax,    	%eax
	js    retc
	movl  %eax,    	fd
	jmp   fwrite 

ropen:
	movl  $5,      	%eax
	movl  $fname, 	%ebx
	movl  $00,  	%ecx 
	movl  $0444,   	%edx	
	int   $0x80
	test  %eax,    	%eax
	js    retc
	movl  %eax,    	fd
	jmp   fread 
	
aopen:
	movl  $5,      	%eax
	movl  $fname,  	%ebx
	movl  $02101,  	%ecx 
	movl  $0644,   	%edx	
	int   $0x80
	test  %eax,    	%eax
	js    retc
	movl  %eax,    	fd

fwrite:
	movl  $4,      	%eax
	movl  fd,      	%ebx
	movl  $output, 	%ecx
	movl  $42,	  	%edx
	int   $0x80
	test  %eax,    	%eax
	js    retc
	jmp   closec

fread:
	movl  $3,      	%eax
	movl  fd,      	%ebx
	movl  $buffer, 	%ecx
	movl  $42,     	%edx
	int   $0x80
	test  %eax,    	%eax
	js    retc

twrite:
	movl  $4,      	%eax
	movl  $1,      	%ebx
	movl  $buffer, 	%ecx
	movl  $42,     	%edx
	int   $0x80
	test  %eax,		%eax
	js    retc	

closec:
	movl  $6,      	%eax
	movl  fd,      	%ebx
	int   $0x80

retc:
	movl  %ebp, 	%esp
	popl  %ebp
	ret

.type read_file, @function
.globl read_file
read_file:
	pushl %ebp
	movl  %esp, 	%ebp

	movl  $5,      	%eax
	movl  $fname, 	%ebx
	movl  $00,     	%ecx 
	movl  $0444,   	%edx	
	int   $0x80
	test  %eax,    	%eax
	js    retw 
	movl  %eax,    	fd
	
floopw:
	movl  $3,      	%eax
	movl  fd,      	%ebx
	movl  $buffer, 	%ecx
	movl  $42,     	%edx
	int   $0x80
	test  %eax,    	%eax
	js    retw 

	movl  $4,      	%eax
	movl  $1,      	%ebx
	movl  $buffer, 	%ecx
	movl  $42,     	%edx
	int   $0x80
	test  %eax,    	%eax
	js    retw	

closew:
	movl  $6,      	%eax
	movl  fd,      	%ebx
	int   $0x80

retw:
	movl  %ebp,		%esp
	popl  %ebp
	ret

.type mmap_file, @function
.globl mmap_file
mmap_file:
	pushl %ebp
	movl  %esp,		%ebp

	movl  $5,      	%eax
	movl  $fname, 	%ebx
	movl  $0102,   	%ecx 
	movl  $0644,   	%edx	
	int   $0x80
	test  %eax,    	%eax
	js    retm 
	movl  %eax,    	fd

	pushl fd
	call  sizefunc
	movl  %eax,   	size
	addl  $4,     	%esp

	pushl $0
	pushl fd
	pushl $1
	pushl $3
	pushl size
	pushl $0
	movl  %esp,   	%ebx
	movl  $90,    	%eax
	int   $0x80
	#test %eax,   	%eax
	#js   retm
	movl  %eax,   	mapfd
	addl  $24,    	%esp
	
	pushl mapfd
	pushl size
	call  convert
	addl  $8,     	%esp
	
	movl  $91,    	%eax
	movl  mapfd,  	%ebx
	movl  size,   	%ecx
	int   $0x80
	test  %eax,   	%eax
	jnz   retm

closem:
	movl  $6,     	%eax
	movl  fd,     	%ebx
	int   $0x80	

retm:
	movl  %ebp, 	%esp
	popl  %ebp
	ret

.type sizefunc, @function
.globl sizefunc
sizefunc:
	pushl %ebp
	movl  %esp,		%ebp
	subl  $8,		%esp
	pushl %edi
	pushl %esi
	pushl %ebx

	movl  $140,		%eax
	movl  8(%ebp),	%ebx
	movl  $0,		%ecx
	movl  $0,		%edx
	leal  -8(%ebp),	%esi
	movl  $2,		%edi
	int   $0x80
	movl  -8(%ebp),	%eax

	popl  %ebx
	popl  %esi
	popl  %edi
	movl  %ebp, 	%esp
	popl  %ebp
	ret

.type convert, @function
.globl convert
convert:
	pushl %ebp
	movl  %esp,		%ebp
	pushl %esi
	pushl %edi

	movl  12(%ebp),	%esi
	movl  %esi,		%edi
	movl  8(%ebp),	%ecx

loopc:
	lodsb
	cmpb  $0x61,	%al
	jl    skip
	cmpb  $0x7a,	%al
	jg    skip
	subb  $0x20,	%al

skip:
	stosb
	loop  loopc

	pop   %edi
	pop   %esi
	movl  %ebp, 	%esp
	popl  %ebp
	ret
