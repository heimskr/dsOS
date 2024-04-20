	.file	"paging.cpp"
	.text
	.globl	_ZN4Boot8findFreeEi
	.type	_ZN4Boot8findFreeEi, @function
_ZN4Boot8findFreeEi:
	pushl	%ebp
	movl	$_bitmap_end, %edx
	subl	$_bitmap_start, %edx
	addl	%edx, %edx
	movl	%esp, %ebp
	shrl	$5, %edx
	pushl	%edi
	movl	$1, %edi
	pushl	%esi
	pushl	%ebx
	pushl	%ecx
	movl	8(%ebp), %eax
	movl	%edx, -16(%ebp)
.L2:
	movl	-16(%ebp), %esi
	cmpl	%esi, %eax
	jnb	.L16
	movl	_bitmap_start(,%eax,4), %ebx
	cmpl	$-1, %ebx
	je	.L3
	movl	%ebx, %ecx
	andl	$1, %ecx
	jne	.L5
	xorl	%ecx, %ecx
.L4:
	sall	$5, %eax
	movzbl	%cl, %ecx
	addl	%ecx, %eax
	jmp	.L1
.L7:
	movl	%edi, %edx
	leal	1(%ecx), %esi
	sall	%cl, %edx
	testl	%ebx, %edx
	je	.L4
	movl	%esi, %ecx
.L5:
	cmpl	$32, %ecx
	jne	.L7
.L3:
	incl	%eax
	jmp	.L2
.L16:
	orl	$-1, %eax
.L1:
	popl	%edx
	popl	%ebx
	popl	%esi
	popl	%edi
	popl	%ebp
	ret
	.size	_ZN4Boot8findFreeEi, .-_ZN4Boot8findFreeEi
	.globl	_ZN4Boot8markUsedEj
	.type	_ZN4Boot8markUsedEj, @function
_ZN4Boot8markUsedEj:
	pushl	%ebp
	movl	$1, %eax
	movl	%esp, %ebp
	movl	8(%ebp), %ecx
	popl	%ebp
	movl	%ecx, %edx
	sall	%cl, %eax
	shrl	$5, %edx
	orl	%eax, _bitmap_start(,%edx,4)
	ret
	.size	_ZN4Boot8markUsedEj, .-_ZN4Boot8markUsedEj
	.type	_ZN4Boot27allocateFreePhysicalAddressEj.part.0, @function
_ZN4Boot27allocateFreePhysicalAddressEj.part.0:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%ebx
	pushl	%eax
	pushl	$1
	call	_ZN4Boot8findFreeEi
	popl	%edx
	movl	%eax, %ebx
	cmpl	$-1, %eax
	je	.L23
	subl	$12, %esp
	movd	%ebx, %xmm0
	pushl	%eax
	call	_ZN4Boot8markUsedEj
	movl	%ebx, %eax
	addl	$16, %esp
	cltd
	movl	$_kernel_physical_start, %eax
	movd	%edx, %xmm1
	cltd
	punpckldq	%xmm1, %xmm0
	movd	%edx, %xmm2
	movd	%eax, %xmm1
	psllq	$21, %xmm0
	punpckldq	%xmm2, %xmm1
	paddq	%xmm1, %xmm0
	movd	%xmm0, %eax
	psrlq	$32, %xmm0
	movd	%xmm0, %edx
	jmp	.L19
.L23:
	xorl	%eax, %eax
	xorl	%edx, %edx
.L19:
	movl	-4(%ebp), %ebx
	leave
	ret
	.size	_ZN4Boot27allocateFreePhysicalAddressEj.part.0, .-_ZN4Boot27allocateFreePhysicalAddressEj.part.0
	.globl	_ZN4Boot8markFreeEj
	.type	_ZN4Boot8markFreeEj, @function
_ZN4Boot8markFreeEj:
	pushl	%ebp
	movl	$-2, %eax
	movl	%esp, %ebp
	movl	8(%ebp), %ecx
	popl	%ebp
	movl	%ecx, %edx
	roll	%cl, %eax
	shrl	$5, %edx
	andl	%eax, _bitmap_start(,%edx,4)
	ret
	.size	_ZN4Boot8markFreeEj, .-_ZN4Boot8markFreeEj
	.globl	_ZN4Boot6isFreeEj
	.type	_ZN4Boot6isFreeEj, @function
_ZN4Boot6isFreeEj:
	pushl	%ebp
	movl	$1, %eax
	movl	%esp, %ebp
	movl	8(%ebp), %ecx
	popl	%ebp
	movl	%ecx, %edx
	sall	%cl, %eax
	shrl	$5, %edx
	andl	_bitmap_start(,%edx,4), %eax
	setne	%al
	ret
	.size	_ZN4Boot6isFreeEj, .-_ZN4Boot6isFreeEj
	.globl	_ZN4Boot27allocateFreePhysicalAddressEj
	.type	_ZN4Boot27allocateFreePhysicalAddressEj, @function
_ZN4Boot27allocateFreePhysicalAddressEj:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%edi
	pushl	%esi
	pushl	%ebx
	subl	$12, %esp
	movl	8(%ebp), %esi
	testl	%esi, %esi
	je	.L33
	orl	$-1, %ebx
	cmpl	$1, %esi
	jne	.L31
	leal	-12(%ebp), %esp
	popl	%ebx
	popl	%esi
	popl	%edi
	popl	%ebp
	jmp	_ZN4Boot27allocateFreePhysicalAddressEj.part.0
.L32:
	subl	$12, %esp
	leal	(%ebx,%edi), %eax
	pushl	%eax
	call	_ZN4Boot6isFreeEj
	addl	$16, %esp
	testb	%al, %al
	jne	.L40
.L31:
	leal	1(%ebx), %eax
	subl	$12, %esp
	movl	$1, %edi
	pushl	%eax
	call	_ZN4Boot8findFreeEi
	addl	$16, %esp
	movl	%eax, %ebx
	jmp	.L32
.L40:
	incl	%edi
	cmpl	%esi, %edi
	jb	.L32
	subl	$12, %esp
	movd	%ebx, %xmm0
	pushl	%ebx
	call	_ZN4Boot8markUsedEj
	movl	%ebx, %eax
	addl	$16, %esp
	cltd
	movl	$_kernel_physical_start, %eax
	movd	%edx, %xmm1
	cltd
	punpckldq	%xmm1, %xmm0
	movd	%edx, %xmm2
	movd	%eax, %xmm1
	psllq	$21, %xmm0
	punpckldq	%xmm2, %xmm1
	paddq	%xmm1, %xmm0
	movd	%xmm0, %eax
	psrlq	$32, %xmm0
	movd	%xmm0, %edx
	jmp	.L29
.L33:
	xorl	%eax, %eax
	xorl	%edx, %edx
.L29:
	leal	-12(%ebp), %esp
	popl	%ebx
	popl	%esi
	popl	%edi
	popl	%ebp
	ret
	.size	_ZN4Boot27allocateFreePhysicalAddressEj, .-_ZN4Boot27allocateFreePhysicalAddressEj
	.globl	_ZN4Boot8allocateEPVyy
	.type	_ZN4Boot8allocateEPVyy, @function
_ZN4Boot8allocateEPVyy:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%edi
	pushl	%esi
	pushl	%ebx
	subl	$12, %esp
	movq	12(%ebp), %xmm0
	movaps	%xmm0, %xmm1
	movaps	%xmm0, %xmm2
	psrlq	$32, %xmm1
	psrlq	$30, %xmm2
	movd	%xmm1, %ebx
	psrlq	$21, %xmm0
	movd	%xmm2, %edi
	shrl	$4, %ebx
	movd	%xmm0, %esi
	andw	$511, %di
	andl	$4088, %ebx
	addl	8(%ebp), %ebx
	andw	$511, %si
	movl	(%ebx), %eax
	movl	4(%ebx), %edx
	andl	$1, %eax
	testb	%al, %al
	jne	.L42
	call	_ZN4Boot27allocateFreePhysicalAddressEj.part.0
	testl	%edx, %edx
	jne	.L54
	testl	%eax, %eax
	je	.L43
.L54:
	andl	$-4096, %eax
	orl	$3, %eax
	movl	%eax, (%ebx)
	movl	%edx, 4(%ebx)
	jmp	.L42
.L43:
	subl	$12, %esp
	pushl	$str_nfp
	call	printf
	addl	$16, %esp
.L45:
/APP
# 102 "32/paging.cpp" 1
	hlt
# 0 "" 2
/NO_APP
	jmp	.L45
.L42:
	movl	(%ebx), %eax
	movzwl	%di, %edi
	movl	4(%ebx), %edx
	andl	$-4096, %eax
	leal	(%eax,%edi,8), %ebx
	movl	(%ebx), %eax
	movl	4(%ebx), %edx
	andl	$1, %eax
	testb	%al, %al
	jne	.L46
	call	_ZN4Boot27allocateFreePhysicalAddressEj.part.0
	testl	%edx, %edx
	jne	.L55
	testl	%eax, %eax
	je	.L47
.L55:
	andl	$-4096, %eax
	orl	$3, %eax
	movl	%eax, (%ebx)
	movl	%edx, 4(%ebx)
	jmp	.L46
.L47:
	subl	$12, %esp
	pushl	$str_nfp
	call	printf
	addl	$16, %esp
.L49:
/APP
# 113 "32/paging.cpp" 1
	hlt
# 0 "" 2
/NO_APP
	jmp	.L49
.L46:
	movl	(%ebx), %eax
	movzwl	%si, %esi
	movl	4(%ebx), %edx
	andl	$-4096, %eax
	leal	(%eax,%esi,8), %ebx
	movl	(%ebx), %eax
	movl	4(%ebx), %edx
	andl	$1, %eax
	testb	%al, %al
	jne	.L41
	call	_ZN4Boot27allocateFreePhysicalAddressEj.part.0
	testl	%edx, %edx
	jne	.L56
	testl	%eax, %eax
	je	.L51
.L56:
	andl	$-4096, %eax
	orb	$-125, %al
	movl	%eax, (%ebx)
	movl	%edx, 4(%ebx)
	jmp	.L41
.L51:
	subl	$12, %esp
	pushl	$str_nfp
	call	printf
	addl	$16, %esp
.L53:
/APP
# 124 "32/paging.cpp" 1
	hlt
# 0 "" 2
/NO_APP
	jmp	.L53
.L41:
	leal	-12(%ebp), %esp
	popl	%ebx
	popl	%esi
	popl	%edi
	popl	%ebp
	ret
	.size	_ZN4Boot8allocateEPVyy, .-_ZN4Boot8allocateEPVyy
	.globl	setup_paging
	.type	setup_paging, @function
setup_paging:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$20, %esp
	pushl	$str_nfp
	call	printf
	addl	$16, %esp
	leave
	ret
	.size	setup_paging, .-setup_paging
	.ident	"GCC: (GNU) 13.2.0"
