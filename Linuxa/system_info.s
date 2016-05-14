###############################################################################
# Show the system information. 
###############################################################################
.section .data
express:
	.asciz "  1: show the 'hello, world!'\n  2: show the system param\n  3: show the system env\n  4: show the user info\n  5: system support cpuid\n  6: show the smid info\n  7: show the CPU type\n\n"
eout:
	.asciz "Input error, please input value [1 - 7]!\n"
output1:
	.asciz "hello, world!\n"
output2:
	.asciz "There are %d parameters:\n"
output3:
	.asciz "The processor Vendor ID is '%s'\n"
output4:
	.asciz "%s\n"
gotcpuid:
	.asciz "This processor supports the CPUID instruction\n"
gotnocpuid:
	.asciz "This processor does not support the CPUID instruction\n"
userinfo:
	.asciz "The user pid is %d, uid is %d, gid is %d, uptime is %d, totalram is %d, freeram is %d, totalswap is %d, freeswap is %d, procs is %d\n"
gotmmx:
	.asciz "This processor supports MMX instruction"
gotsse:
	.asciz "This processor supports SSE instruction"
gotsse2:
	.asciz "This processor supports SSE2 instruction"
gotsse3:
	.asciz "This processor supports SSE3 instruction"
result:
uptime:
	.int   0
load1:
	.int   0
load5:
	.int   0
load15:
	.int   0
totalram:
	.int   0
freeram:
	.int   0
sharedram:
	.int   0
bufferram:
	.int   0
totalswap:
	.int   0
freeswap:
	.int   0
procs:
	.byte  0x00,  0x00
totalhigh:
	.int   0
memunit:
	.int   0
others:
	.fill  128

.section .rodata
sysacs:
	.equ LINUX_SYS_CALL, 0x80

.section .bss
	.lcomm  bufs,	  4
	.lcomm  pid,      4
	.lcomm  uid,      4
	.lcomm  gid,      4
	.lcomm  buf,      12
	.lcomm  ecxdata,  4
	.lcomm  edxdata,  4
other:
	.fill 128

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
	call  hello_info 
	jmp   end
	
func2:
	cmp   $2, 	bufs
	jne   func3 
	call  param_info 
	jmp   end

func3:
	cmp   $3, 	bufs
	jne   func4 
	call  env_info 
	jmp   end

func4:
	cmp   $4, 	bufs
	jne   func5
	call  user_info 
	jmp   end

func5:
	cmp   $5, 	bufs
	jne   func6 
	call  cpuid_info  
	jmp   end
	
func6:
	cmp   $6, 	bufs
	jne   func7 
	call  smid_info
	jmp   end

func7:
	cmp   $7, 	bufs
	jne   errors 
	call  cpu_info 
	jmp   end

errors:
	pushl $eout
	call  printf

end:	
	movl  $1, 	%eax
	movl  $0,   %ebx
	int   $0x80

.type hello_info, @function
.globl hello_info
hello_info:
	pushl %ebp
	movl  %esp,		%ebp

    movl  $4,       %eax
	movl  $1,       %ebx
	movl  $output1, %ecx
	movl  $14,      %edx
	int   $LINUX_SYS_CALL
		
	movl  %ebp,     %esp
	popl  %ebp
	ret

.type param_info, @function
.globl param_info
param_info:
	pushl %ebp
	movl  %esp,   	%ebp
	
	jmp   showp
	movl  (%esp), 	%ecx
	pushl %ecx
	pushl $output2
	call  printf
	addl  $4,     	%esp
	popl  %ecx
	movl  %esp,   	%ebp
	addl  $4,     	%ebp

loopc:
	pushl %ecx
	pushl (%ebp)
	pushl $output4
	call  printf
	addl  $8,    	%esp
	popl  %ecx
	addl  $4,    	%ebp
	loop  loopc
	jmp   retp

showp:
	addl  $12,   	%ebp

loopp:  
	cmpl  $0,    	(%ebp)
	je    retp 
	pushl (%ebp)
	pushl $output4
	call  printf
	addl  $12,   	%esp
	addl  $4,    	%ebp
	loop  loopp

retp:
	movl  $1, 		%eax
	movl  $0,   	%ebx
	int   $0x80

.type env_info, @function
.globl env_info
env_info:
	pushl %ebp
	movl  %esp,   	%ebp

	addl  $24, 		%ebp
loope:
	cmpl  $0, 		(%ebp)
	je    endit
	pushl (%ebp)
	pushl $output4
	call  printf
	addl  $12, 		%esp
	addl  $4, 		%ebp
	loop  loope

endit:	
	movl  $1, 		%eax
	movl  $0,   	%ebx
	int   $0x80

.type user_info, @function
.globl user_info
user_info:
	pushl %ebp
	movl  %esp,    	%ebp

	movl  $20,     	%eax
	int   $LINUX_SYS_CALL
	movl  %eax,    	pid

	movl  $24,     	%eax
	int   $LINUX_SYS_CALL
	movl  %eax,    	uid

	movl  $47,     	%eax
	int   $LINUX_SYS_CALL
	movl  %eax,    	gid

	movl  $result, 	%ebx
	movl  $116,    	%eax
	int   $LINUX_SYS_CALL

showu:
	pushl procs
	pushl freeswap
	pushl totalswap
	pushl freeram
	pushl totalram
	pushl uptime
	pushl gid
	pushl uid
	pushl pid
	pushl $userinfo
	call  printf
	addl  $40,   	%esp

	movl  %ebp,    	%esp
	popl  %ebp
	ret

.type cpuid_info, @function
.globl cpuid_info
cpuid_info:
	pushl %ebp
	movl  %esp, 	%ebp

	pushfl
	popl  %eax
	movl  %eax, 	%edx
	xor   $0x00200000, %eax
	pushl %eax
	popfl
	pushfl
	popl  %eax
	xor   %edx, 	%eax
	test  $0x00200000, %eax
	jnz   cpuids
	pushl $gotnocpuid
	call  printf
	add   $4,   	%esp
	jmp   retc

cpuids:
	pushl $gotcpuid
	call  printf
	add   $4,   	%esp

retc:
	movl  %ebp, 	%esp
	popl  %ebp
	ret

.type smid_info, @function
.globl smid_info
smid_info:
	pushl %ebp
	movl  %esp,    	%ebp

	movl  $1,      	%eax
	cpuid 
	movl  %ecx,    	ecxdata
	movl  %edx,    	edxdata
	
	test  $0x00800000, %edx
	jz    rets 
	pushl $gotmmx
	pushl $output4
	call  printf
	addl  $8,      	%esp

	movl  edxdata, 	%edx
	test  $0x02000000, %edx
	jz    rets 
	pushl $gotsse
	pushl $output4
	call  printf
	addl  $8,      	%esp
	
	movl  edxdata, 	%edx
	test  $0x04000000, %edx
	jz    rets 
	pushl $gotsse2
	pushl $output4
	call  printf
	addl  $8,      	%esp

	movl  ecxdata, 	%ecx
	test  $0x00000001, %edx
	jz    rets 
	pushl $gotsse3
	pushl $output4
	call  printf
	addl  $8,      	%esp

rets:
	movl  %ebp,    	%esp
	popl  %ebp
	ret

.type cpu_info, @function
.globl cpu_info
cpu_info:
	pushl %ebp
	movl  %esp, 	%ebp

	movl  $0,   	%eax
	cpuid
	movl  $buf, 	%edi
	movl  %ebx, 	(%edi)
	movl  %edx, 	4(%edi)
	movl  %ecx, 	8(%edi)
	pushl $buf
	pushl $output3
	call  printf
	addl  $8,   	%esp

	movl  %ebp, 	%esp
	popl  %ebp
	ret
