.macro pushall
	push %rax
	push %rbx
	push %rcx
	push %rdx
	push %rsp
	push %rbp
	push %rsi
	push %rdi
.endm

.macro popall
	pop %rdi
	pop %rsi
	pop %rbp
	pop %rsp
	pop %rdx
	pop %rcx
	pop %rbx
	pop %rax
.endm

.macro isr_common id, fn
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
	sti
	iretq
.endm

.macro isr_errc id, fn
.global isr_\id
.type isr_\id , @function
isr_\id :
	cli
	pushw $0
	pushw $\id
	isr_common \id , \fn
.endm

.macro isr_noerrc id, fn
.global isr_\id
.type isr_\id , @function
isr_\id :
	cli
	pushw $0
	isr_common \id , \fn
.endm

.section .text
	isr_noerrc 0, div0
