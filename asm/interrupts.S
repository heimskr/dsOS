

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
	pushall

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

	popall

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

.section .rodata
fmt32: .asciz "Timer interrupt: %ld\n"

.data
ticks: .8byte 0
.global ticks


.section .text
	isr_noerrc  0, div0
	isr_errc    8, double_fault
	isr_errc   14, page_interrupt
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

.global isr_32
.type isr_32, @function
isr_32:
	cli
	push %rdi
	push %rsi
	push %rax
	mov $fmt32, %rdi
	movq (ticks), %rsi

	mov $0, %rax
	call printf
	movq (apic_base), %rax
	add $0xb0, %rax
	movl $0, (%rax)
	pop %rax
	pop %rsi
	pop %rdi
	incq (ticks)
	iretq