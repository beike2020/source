###############################################################################
# An example of advance math instruction. 
###############################################################################
.section .data
express:
	.asciz "  1: show the fpu status\n  2: show the fpu ctrl flag\n  3: show the fpu stack\n  4: handle fpu float\n  5: handle fpu math\n  6: handle fpu round\n  7: handle fpu prem\n  8: handle fpu sincos\n  9: handle fpu log10\n 10: handle fpu logx\n 11: handle fpu coms\n 12: handle fpu comi\n 13: handle fpu env\n 14: handle fpu flag restore\n 15: handle mm add and cmp\n\n"
eout:
	.asciz "Input error, please input value [1 - 15]!\n"
outputd:
	.asciz "The result is %d!\n"
outputs:
	.asciz "The result is %s!\n"
outputf:
	.asciz "The result is %f!\n"
outputx:
	.asciz "The result is %x!\n"
outputg:
	.asciz "The result is greater!\n"
outputl:
	.asciz "The result is less!\n"
cvalue:
	.byte  0x7f, 0x00
rdown:
	.byte  0x7f, 0x07
rup:
	.byte  0x7f, 0x0b
svalu1:
	.int   40
svalu2:
	.float 92.4405
svalu3:
	.double 221.440321 
mvalu1:
    .int   10, 20
mvalu2:
    .int   30, 40
mvalu3:
    .short 10, 20, -30, 40
mvalu4:
    .short 10, 40, -30, 45
value1:
	.float 43.65
value2:
	.int   22
value3:
	.float 76.34
value4:
	.float 3.1
value5:
	.float 12.43
value6:
	.int   6
value7:
	.float 140.2
value8:
	.float 94.21
degree1:
	.float 90.0
val180:
	.int   180
lownum:
	.float 10.0
highnum:
	.float 12.0
scale1:
	.float 2.0
scale2:
	.float -2.0

.section .bss
	.lcomm bufs,    4
	.lcomm int1,    4
	.lcomm status,  2
	.lcomm control, 2
	.lcomm radian1, 4
	.lcomm result1, 4
	.lcomm result2, 4
	.lcomm result3, 4
	.lcomm result4, 8
	.lcomm fpuenv,  28

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
	call  fpu_status 
	jmp   end
	
func2:
	cmp   $2,   bufs
	jne   func3 
	call  fpu_ctrl 
	jmp   end

func3:
	cmp   $3,   bufs
	jne   func4 
	call  fpu_stack  
	jmp   end

func4:
	cmp   $4,   bufs
	jne   func5
	call  fpu_float 
	jmp   end

func5:
	cmp   $5,   bufs
	jne   func6 
	call  fpu_math  
	jmp   end

func6:
	cmp   $6,   bufs
	jne   func7 
	call  fpu_round 
	jmp   end

func7:
	cmp   $7,   bufs
	jne   func8 
	call  fpu_prem  
	jmp   end

func8:
	cmp   $8,   bufs
	jne   func9
	call  fpu_sincos 
	jmp   end

func9:
	cmp   $9,   bufs
	jne   func10 
	call  fpu_log10  
	jmp   end

func10:
	cmp   $10,  bufs
	jne   func11 
	call  fpu_logx
	jmp   end

func11:
	cmp   $11,  bufs
	jne   func12 
	call  fpu_coms
	jmp   end

func12:
	cmp   $12,  bufs
	jne   func13 
	call  fpu_comi
	jmp   end

func13:
	cmp   $13,  bufs
	jne   func14 
	call  fpu_env
	jmp   end

func14:
	cmp   $14,  bufs
	jne   func15 
	call  fpu_restore 
	jmp   end

func15:
	cmp   $15,  bufs
	jne   errors 
	call  mm_handle 
	jmp   end

errors:
	pushl $eout
	call  printf

end:	
	movl  $1,   %eax
	movl  $0,   %ebx
	int   $0x80

.type  fpu_status, @function
.globl fpu_status 
fpu_status:
	pushl %ebp
	movl  %esp,		%ebp

	fstsw status	
	pushl status
	pushl $outputx
	call  printf
	addl  $8,   	%esp

	movl  %ebp, 	%esp
	popl  %ebp
	ret

.type  fpu_ctrl, @function
.globl fpu_ctrl 
fpu_ctrl:
	pushl %ebp
	movl  %esp, 	%ebp

	fstcw status	
	pushl status
	pushl $outputx
	call  printf
	addl  $8,   	%esp

	fldcw cvalue 
	fstcw status	
	pushl status
	pushl $outputx
	call  printf
	addl  $8,   	%esp

	movl  %ebp, 	%esp
	popl  %ebp
	ret

.type  fpu_stack, @function
.globl fpu_stack 
fpu_stack:
	pushl %ebp
	movl  %esp, 	%ebp

	finit
	fstcw control	
	fstsw status
	filds svalu1
	fists int1
	flds  svalu2
	fldl  svalu3
	fst   %st(4)
	fxch  %st(1)
	subl  $8,   	%esp

	fstpl (%esp)
	pushl $outputf
	call  printf
	addl  $12,  	%esp

	movl  %ebp, 	%esp
	popl  %ebp
	ret

.type  fpu_float, @function
.globl fpu_float 
fpu_float:
	pushl %ebp
	movl  %esp,    	%ebp

	finit
	flds  value1
	fidiv value2
	flds  value3
	flds  value4
	fmul  %st(1),  	%st(0)
	fadd  %st(2),  	%st(0)
	flds  value5
	fimul value6
	flds  value7
	flds  value8
	fdivrp
	fsubr %st(1),  	%st(0)
	fdivr %st(2),  	%st(0)

	subl  $8,      	%esp
	fstpl (%esp)
	pushl $outputf
	call  printf
	add   $12,     	%esp

	movl  %ebp,    	%esp
	popl  %ebp
	ret

.type  fpu_math, @function
.globl fpu_math 
fpu_math:
	pushl %ebp
	movl  %esp,		%ebp

	finit
	flds  value1
	fchs
	subl  $8,      	%esp
	fstpl (%esp)
	pushl $outputf
	call  printf
	add   $12,     	%esp

	flds  value2
	fabs
	subl  $8,      	%esp
	fstpl (%esp)
	pushl $outputf
	call  printf
	add   $12,     	%esp

	flds  value3
	fsqrt	
	subl  $8,      	%esp
	fstpl (%esp)
	pushl $outputf
	call  printf
	add   $12,     	%esp

	movl  %ebp,    	%esp
	popl  %ebp
	ret

.type  fpu_round, @function
.globl fpu_round 
fpu_round:
	pushl %ebp
	movl  %esp,    	%ebp

	finit
	flds  value1
	frndint
	fists result1 

	fldcw rdown
	flds  value1
	frndint
	subl  $8,      	%esp
	fstpl (%esp)
	pushl $outputf
	call  printf
	add   $12,     	%esp

	fldcw rup
	flds  value1
	frndint
	subl  $8,      	%esp
	fstpl (%esp)
	pushl $outputf
	call  printf
	add   $12,     	%esp

	movl  %ebp,    	%esp
	popl  %ebp
	ret

.type  fpu_prem, @function
.globl fpu_prem 
fpu_prem:
	pushl %ebp
	movl  %esp,    	%ebp

	finit
	flds  value7
	flds  value8

looppr:
	fprem1
	fstsw %ax
	testb $4,      	%ah
	jnz   looppr
	fsts  result1

	subl  $8,      	%esp
	fstpl (%esp)
	pushl $outputf
	call  printf
	add   $12,     	%esp

	movl  %ebp,    	%esp
	popl  %ebp
	ret

.type  fpu_sincos, @function
.globl fpu_sincos 
fpu_sincos:
	pushl %ebp
	movl  %esp,    	%ebp

	finit
	flds  degree1
	fidivs val180
	fldpi
	fmul  %st(1), 	%st(0)
	#fsincos
	#fstps result1
	#fsts  result2
	fsts  radian1
	fsin
	fsts  result1

	subl  $8,      	%esp
	fstpl (%esp)
	pushl $outputf
	call  printf
	add   $12,     	%esp

	flds  radian1
	flds  radian1
	fcos
	fsts  result2

	subl  $8,      	%esp
	fstpl (%esp)
	pushl $outputf
	call  printf
	add   $12,     	%esp

	movl  %ebp,    	%esp
	popl  %ebp
	ret

.type  fpu_log10, @function
.globl fpu_log10 
fpu_log10:
	pushl %ebp
	movl  %esp,    	%ebp

	finit
	flds  scale1
	flds  lownum
	fscale
	fsts  result1

	subl  $8,      	%esp
	fstpl (%esp)
	pushl $outputf
	call  printf
	add   $12,     	%esp

	flds  scale2
	flds  lownum
	fscale
	fsts  result2

	subl  $8,      	%esp
	fstpl (%esp)
	pushl $outputf
	call  printf
	add   $12,     	%esp

	movl  %ebp,		%esp
	popl  %ebp
	ret

.type  fpu_logx, @function
.globl fpu_logx
fpu_logx:
	pushl %ebp
	movl  %esp,    	%ebp

	finit
	fld1
	flds  lownum
	fyl2x
	fld1
	fdivp
	flds  highnum
	fyl2x
	fsts  result1

	subl  $8,      	%esp
	fstpl (%esp)
	pushl $outputf
	call  printf
	add   $12,     	%esp

	movl  %ebp,    	%esp
	popl  %ebp
	ret

.type  fpu_coms, @function
.globl fpu_coms
fpu_coms:
	pushl %ebp
	movl  %esp,    	%ebp

	flds  value7
	fcoms value8
	fstsw
	sahf
	ja    greater
	jmp   retc

greater:
	pushl $outputg
	call  printf
	add   $4,     	%esp

retc:
	movl  %ebp,		%esp
	popl  %ebp
	ret

.type  fpu_comi, @function
.globl fpu_comi
fpu_comi:
	pushl %ebp
	movl  %esp,    	%ebp

	finit
	flds  value8
	flds  value7
	fcomi %st(1),  	%st(0)
	fcmovb %st(1), 	%st(0)
	fsts  result1
	ja    big 
	jmp   reti

big:
	subl  $8,      	%esp
	fstpl (%esp)
	pushl $outputf
	call  printf
	add   $12,     	%esp

reti:
	movl  %ebp,    	%esp
	popl  %ebp
	ret

.type  fpu_env, @function
.globl fpu_env
fpu_env:
	pushl %ebp
	movl  %esp,    	%ebp

	finit
	flds  value7
	flds  value8
	fldcw rup
	fstenv fpuenv

	finit
	flds  value2
	flds  value1
	fldenv fpuenv

	subl  $8,      	%esp
	fstpl (%esp)
	pushl $outputf
	call  printf
	add   $12,     	%esp

	movl  %ebp,   	%esp
	popl  %ebp
	ret

.type  fpu_restore, @function
.globl fpu_restore
fpu_restore:
	pushl %ebp
	movl  %esp,    	%ebp

	finit
	flds  value7
	flds  value8
	fldcw rup
	fsave fpuenv

	flds  value2
	flds  value1
	frstor fpuenv

	subl  $8,      	%esp
	fstpl (%esp)
	pushl $outputf
	call  printf
	add   $12,     	%esp

	movl  %ebp,    	%esp
	popl  %ebp
	ret

.type  mm_handle, @function
.globl mm_handle
mm_handle:
	pushl %ebp
	movl  %esp,    	%ebp

    movq  mvalu1,  	%mm0
    movq  mvalu2,  	%mm1
    paddd %mm1,    	%mm0
    movq  %mm0,    	result4

    movq  mvalu3,  	%mm0
    movq  mvalu4,  	%mm1
    pcmpeqw %mm1,  	%mm0
    movq  %mm0,    	result4
    
	movl  %ebp,    	%esp
	popl  %ebp
	ret
