

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



.section .bss
	.comm gpf_addr, 8
	.comm timer_addr, 8

.section .rodata
timerfmt: .asciz "Timer interrupt: 0x%lx\n"
pagefmt: .asciz ":: 0x%lx\n"

.data
ticks: .8byte 0
timer_max: .8byte -1
.global ticks
.global timer_max



.section .text
	isr_noerrc  0, div0
	isr_errc    8, double_fault
	// isr_errc   14, page_interrupt
	// isr_noerrc 33, irq1
	isr_noerrc 39, spurious_interrupt

.global isr_13
.type isr_13, @function
isr_13:
	cli
	push %rax
	movq 16(%rsp), %rax
	mov %rax, gpf_addr
	pop %rax
isr_push_13:
	pushw $0
	pushw $13
	isr_common general_protection_fault
isr_iretq_13:
	add $4, %rsp
	iretq

.global isr_14
.type isr_14, @function
isr_14:
	cli
	push %rax
	mov 16(%rsp), %rax
	push %rdi
	push %rsi
	mov $pagefmt, %rdi
	mov %rax, %rsi
	mov $0, %rax
	call printf
	mov $pagefmt, %rdi
	mov %r14, %rsi
	mov $0, %rax
	call printf
	mov $pagefmt, %rdi
	mov %r15, %rsi
	mov $0, %rax
	call printf
	pop %rsi
	pop %rdi
	pop %rax
	// push $0
	// push $14
	isr_common page_interrupt
	// add $4, %rsp
	iretq

.global isr_32
.type isr_32, @function
isr_32:
	cli
	push %rax
	mov 8(%rsp), %rax
	push %rdi
	push %rsi
	mov $timerfmt, %rdi
	incq (ticks)
	// movq (ticks), %rsi
	mov %rax, %rsi

	mov $0, %rax
	call printf
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

.global isr_33
.type isr_33, @function
isr_33:
	cli
	call irq1
isr_iretq_33:
	iretq