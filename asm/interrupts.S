

.macro pushall
	push %rax
	push %rbx
	push %rcx
	push %rdx
	push %rsp
	push %rbp
	push %rsi
	push %rdi
	push %r8
	push %r9
	push %r10
	push %r11
	push %r12
	push %r13
	push %r14
	push %r15
.endm




.macro popall
	pop %r15
	pop %r14
	pop %r13
	pop %r12
	pop %r11
	pop %r10
	pop %r9
	pop %r8
	pop %rdi
	pop %rsi
	pop %rbp
	pop %rsp
	pop %rdx
	pop %rcx
	pop %rbx
	pop %rax
.endm




.macro isr_common fn
	// pushall

	push %rax
	mov %ds, %ax
	push %rax

	mov $0x10, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

	call \fn

	pop %rax

	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

	pop %rax

	// popall

	add $8, %rsp
.endm




.macro isr_errc id, fn
.global isr_\id
.type isr_\id , @function
isr_\id :
	cli
isr_push_\id :
	pushw $0
	pushw $\id
	isr_common \fn
isr_iretq_\id :
	add $4, %rsp
	iretq
.endm




.macro isr_noerrc id, fn
.global isr_\id
.type isr_\id , @function
isr_\id :
	cli
isr_push_\id :
	pushw $0
	isr_common \fn
isr_iretq_\id :
	iretq
.endm




.macro print1 fmt, reg
	movq \fmt , %rdi
	movq \reg , %rsi
	movq $0, %rax
	call printf
.endm

.macro print0 fmt
	movq \fmt , %rdi
	movq $0, %rax
	call printf
.endm




.section .bss
	.comm gpf_addr, 8
	.comm timer_addr, 8

.section .rodata
timerfmt: .asciz "Timer interrupt: 0x%lx\n"
pagefmt: .asciz ":: 0x%lx\n"
raxfmt: .asciz "%%rax: 0x%lx\n"
rbxfmt: .asciz "%%rbx: 0x%lx\n"
rcxfmt: .asciz "%%rcx: 0x%lx\n"
rdxfmt: .asciz "%%rdx: 0x%lx\n"
rdifmt: .asciz "%%rdi: 0x%lx\n"
rsifmt: .asciz "%%rsi: 0x%lx\n"
addrfmt: .asciz "addr: 0x%lx\n"
gpffmt: .asciz "General protection fault!\n"

.data
ticks: .8byte 0
timer_max: .8byte -1
.global ticks
.global timer_max



.section .text
	isr_noerrc  0, div0
	isr_errc    8, double_fault
	// isr_errc   14, page_interrupt
	isr_noerrc 33, irq1
	isr_noerrc 39, spurious_interrupt

.global isr_13
.type isr_13, @function
isr_13:
	cli
	push %rax
	push %rbx
	push %rcx
	push %rdx
	push %rdi
	push %rsi
	print0 $gpffmt
	print1 $addrfmt, 56(%rsp)
	print1 $raxfmt, 48(%rsp)
	print1 $rbxfmt, 40(%rsp)
	print1 $rcxfmt, 32(%rsp)
	print1 $rdxfmt, 24(%rsp)
	print1 $rdifmt, 16(%rsp)
	print1 $rsifmt, 8(%rsp)
	pop %rsi
	pop %rdi
	pop %rdx
	pop %rcx
	pop %rbx
	pop %rax
	1: jmp 1b
	iretq

.global isr_14
.type isr_14, @function
isr_14:
	cli
	push %rax
	mov 16(%rsp), %rax
	push %rdi
	push %rsi
	print1 $pagefmt, %rax
	pop %rsi
	pop %rdi
	pop %rax
	isr_common page_interrupt
	iretq

.global isr_32
.type isr_32, @function
isr_32:
	cli
	push %rax
	mov 8(%rsp), %rax
	push %rdi
	push %rsi
	incq (ticks)

	// print1 $timerfmt, (ticks)
	print1 $timerfmt, %rax

	movq (apic_base), %rax
	add $0xb0, %rax
	movl $0, (%rax)
	pop %rsi
	pop %rdi

	// If timer_max is -1, don't compare it to ticks.
	cmpq $-1, (timer_max)
	je isr_32_timer_check_done

	// If timer_max != ticks, don't call timer_addr.
	movq (ticks), %rax
	cmpq %rax, (timer_max)
	jne isr_32_timer_check_done

	// If timer_max isn't -1 and timer_max == ticks, jump to the function whose pointer is stored in timer_addr.
	movq (timer_addr), %rax
	call *%rax

isr_32_timer_check_done:
	pop %rax
	iretq

// .global isr_33
// .type isr_33, @function
// isr_33:
// 	cli
// 	call irq1
// isr_iretq_33:
// 	iretq