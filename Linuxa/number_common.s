###############################################################
# An example of using MMX, SSE and SSE2 arithmetic instructions. 
###############################################################
.section .data
express:
		.asciz "  1: test the signed integer\n  2: test the unsigned int expand\n  3: test the signed int expand\n  4: test the bad int\n  5: test the float handle\n  6: test the float constants\n  7: test the mmx arithmetic\n  8: test the sse arithmetic\n  9: test the sse cmp\n 10: test the sse quad\n 11: test the sse float\n 12: test the sse2 math\n 13: test the sse2 float\n\n"
eout:
		.asciz "Input error, please input value [1 - 13]!\n"
result:
    .asciz "The value is %d\n"
.align 16
value1:
		.float 12.34, 2345., -93.2, 10.44
value2:
		.float 39.234, 21,4, 100.94, 10.56
value3:
		.float 12.34, 21.4, -93.2, 10.45
.align 16
value4:
		.double 10.42, -5.330
value5:
		.double 4.25, 2.10
value6:
		.int 10, 20, 30, 40
value7:
		.int 5, 15, 25, 35
value8:
		.quad 1, -1
data1:
		.byte 0x34, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
data2:
		.int 2
data3:
		.float 12.34
data4:
		.double 2353.631

.section .bss
		.lcomm bufs,   4
		.lcomm data,   8
		.lcomm ret1,   16   
		.lcomm ret2,   16   
		.lcomm ret3,   16

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
		call  sint_base 
		jmp   end
		
func2:
		cmp   $2,   bufs
		jne   func3 
		call  uint_expand 
		jmp   end

func3:
		cmp   $3,   bufs
		jne   func4 
		call  sint_expand  
		jmp   end

func4:
		cmp   $4,   bufs
		jne   func5 
		call  bsd_int 
		jmp   end

func5:
		cmp   $5,   bufs
		jne   func6 
		call  float_base 
		jmp   end

func6:
		cmp   $6,   bufs
		jne   func7 
		call  float_constants 
		jmp   end

func7:
		cmp   $7,   bufs
		jne   func8 
		call  mmx_math  
		jmp   end

func8:
		cmp   $8,   bufs
		jne   func9 
		call  sse_math 
		jmp   end

func9:
		cmp   $9,   bufs
		jne   func10 
		call  sse_cmp  
		jmp   end

func10:
		cmp   $10,   bufs
		jne   func11
		call  sse_quad 
		jmp   end

func11:
		cmp   $11,   bufs
		jne   func12 
		call  sse_float  
		jmp   end

func12:
		cmp   $12,   bufs
		jne   func13 
		call  sse2_math  
		jmp   end

func13:
		cmp   $13,   bufs
		jne   errors 
		call  sse2_float 
		jmp   end

errors:
		pushl $eout
		call  printf

end:		
		movl  $1,   %eax
		movl  $0,   %ebx
		int   $0x80

.type  sint_base, @function
.globl sint_base 
sint_base:
		pushl %ebp
		movl  %esp, 	%ebp

		movl  $-345, 	%ecx
		movw  $0xffb1, 	%dx
		movl  $-45, 	%ebx

		pushl %ebx
		pushl $result
		call  printf
		addl  $8,      	%esp	

		movl  %ebp, 	%esp
		popl  %ebp
		ret

.type  uint_expand, @function
.globl uint_expand 
uint_expand:
		pushl %ebp
		movl  %esp,    	%ebp

		movl  $279, 	%ecx
		movzx %cl, 		%ebx
		pushl %ebx
		pushl $result
		call  printf
		addl  $8,      	%esp	
		
		movl  %ebp,    	%esp
		popl  %ebp
		ret

.type  sint_expand, @function
.globl sint_expand 
sint_expand:
		pushl %ebp
		movl  %esp, 	%ebp

		movw  $-79, 	%cx
		movl  $0,   	%ebx
		movsx %cx,  	%ebx
		pushl %ebx
		pushl $result
		call  printf
		addl  $8,      	%esp	

		movl  %ebp, 	%esp
		popl  %ebp
		ret

.type  bsd_int, @function
.globl bsd_int 
bsd_int:
		pushl %ebp
		movl  %esp,    	%ebp

		fbld  data1
		fimul data2
		fbstp data1
		pushl data1
		pushl $result
		call  printf
		addl  $8,      %esp	
		
		movl  %ebp,    %esp
		popl  %ebp
		ret

.type  float_base, @function
.globl float_base 
float_base:
		pushl %ebp
		movl  %esp,    %ebp

		flds  data3
		fldl  data4
		fstl  data
		pushl data
		pushl $result
		call  printf
		addl  $8,      %esp	
		
		movl  %ebp,    %esp
		popl  %ebp
		ret

.type  float_constants, @function
.globl float_constants 
float_constants:
		pushl %ebp
		movl  %esp,    %ebp

		fld1
		fldl2t
		fldl2e
		fldpi
		fldlg2
		fldln2
		fldz
		
		movl  %ebp,    %esp
		popl  %ebp
		ret

.type mmx_math, @function
.globl mmx_math
mmx_math:
		pushl   %ebp
		movl    %esp,   %ebp

		movq    value1, %mm0
		movq    value2, %mm1
		paddd   %mm1,   %mm0
		movq    %mm0,   ret1

		movq    value3, %mm0
		movq    value4, %mm1
		pcmpeqw %mm1,   %mm0
		movq    %mm0,   ret1
				
		movl    %ebp,   %esp
		popl    %ebp
		ret

.type sse_math, @function
.globl sse_math
sse_math:
		pushl  %ebp
		movl   %esp,    %ebp

		movaps value1,  %xmm0
		movaps value2,  %xmm1
		addps  %xmm1,   %xmm0
		sqrtps %xmm0,   %xmm0
		maxps  %xmm1,   %xmm0
		movaps %xmm0,   ret1
				
		movl   %ebp,    %esp
		popl   %ebp
		ret

.type sse_cmp, @function
.globl sse_cmp
sse_cmp:
		pushl   %ebp
		movl    %esp,   %ebp

		movaps  value1, %xmm0
		movaps  value2, %xmm1
		cmpeqps %xmm1,  %xmm0
		movaps  %xmm0,	ret1
				
		movl    %ebp,   %esp
		popl    %ebp
		ret

.type sse_quad, @function
.globl sse_quad
sse_quad:
		pushl  %ebp
		movl   %esp,    %ebp

		movdqu value7,  %xmm0
		movdqu value8,  %xmm1
				
		movl   %ebp,    %esp
		popl   %ebp
		ret

.type sse_float, @function
.globl sse_float
sse_float:
		pushl  %ebp
		movl   %esp,    %ebp

		movups value1,  %xmm0
		movups value2,  %xmm1
		movups %xmm0,   %xmm2
		movups %xmm0,   ret3
				
		movl   %ebp,    %esp
		popl   %ebp
		ret

.type sse2_math, @function
.globl sse2_math
sse2_math:
		pushl  %ebp
		movl   %esp,    %ebp

		movapd value4,  %xmm0
		movapd value5,  %xmm1
		movdqa value6,  %xmm2
		movdqa value7,  %xmm3
		mulpd  %xmm1,   %xmm0
		paddd  %xmm3,   %xmm2
		movapd %xmm0,   ret2
		movdqa %xmm2,   ret3
				
		movl   %ebp,    %esp
		popl   %ebp
		ret
		
.type sse2_float, @function
.globl sse2_float
sse2_float:
		pushl  %ebp
		movl   %esp,    %ebp

		movupd value4,  %xmm0
		movupd value5,  %xmm1
		movupd %xmm0,   %xmm2
		movupd %xmm0,   ret3
				
		movl   %ebp,    %esp
		popl   %ebp
		ret

