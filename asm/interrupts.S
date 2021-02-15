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
	pushw $0
	pushw $\id
	isr_common \fn
	add $4, %rsp
	sti
	iretq
.endm



.macro isr_noerrc id, fn
.global isr_\id
.type isr_\id , @function
isr_\id :
	cli
	pushw $0
	isr_common \fn
	sti
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
r8fmt: .asciz "%%r8: 0x%lx\n"
r9fmt: .asciz "%%r9: 0x%lx\n"
r10fmt: .asciz "%%r10: 0x%lx\n"
r11fmt: .asciz "%%r11: 0x%lx\n"
r12fmt: .asciz "%%r12: 0x%lx\n"
r13fmt: .asciz "%%r13: 0x%lx\n"
r14fmt: .asciz "%%r14: 0x%lx\n"
r15fmt: .asciz "%%r15: 0x%lx\n"
rbpfmt: .asciz "%%rbp: 0x%lx\n"
rspfmt: .asciz "%%rsp: 0x%lx\n"
csfmt: .asciz "%%cs: 0x%x\n"
dsfmt: .asciz "%%ds: 0x%x\n"
esfmt: .asciz "%%es: 0x%x\n"
fsfmt: .asciz "%%fs: 0x%x\n"
gsfmt: .asciz "%%gs: 0x%x\n"
addrfmt: .asciz "addr: 0x%lx\n"
gpffmt: .asciz "General protection fault!\n"

.data
ticks: .8byte 0
timer_max: .8byte -1
isr_rbp: .8byte 0
addr8: .8byte 0
addr14: .8byte 0
// last_scancode: .byte 0
.global ticks
.global timer_max
.global isr_rbp
.global addr8
.global addr14
// .global last_scancode



.section .text
	isr_noerrc  0, div0
	// isr_errc    8, double_fault
	isr_noerrc 39, spurious_interrupt
	isr_noerrc 43, irq11
	isr_noerrc 46, irq14
	isr_noerrc 47, irq15

.global isr_8
.type isr_8, @function
isr_8:
	cli
	mov %rbp, (isr_rbp)
	push %rax
	movq 16(%rsp), %rax
	mov %rax, (addr8)
	pop %rax


	pushall
	mov (isr_rbp), %rdi
	call _ZN5Thorn6Kernel9backtraceEPm
	popall

	isr_common double_fault
	iretq

.global isr_13
.type isr_13, @function
isr_13:
	cli
	// mov %rbp, (isr_rbp)
	push %rax
	push %rbx
	push %rcx
	push %rdx
	push %rdi
	push %rsi
	push %r8
	push %r9
	push %r10
	push %r11
	push %r12
	push %r13
	push %r14
	push %r15

	// mov (isr_rbp), %rdi
	// call _ZN5Thorn6Kernel9backtraceEPm

	print0 $gpffmt
	print1 $addrfmt, 120(%rsp)
	print1 $raxfmt, 104(%rsp)
	print1 $rbxfmt, 96(%rsp)
	print1 $rcxfmt, 88(%rsp)
	print1 $rdxfmt, 80(%rsp)
	print1 $rdifmt, 72(%rsp)
	print1 $rsifmt, 64(%rsp)
	print1 $r8fmt, 56(%rsp)
	print1 $r9fmt, 48(%rsp)
	print1 $r10fmt, 40(%rsp)
	print1 $r11fmt, 32(%rsp)
	print1 $r12fmt, 24(%rsp)
	print1 $r13fmt, 16(%rsp)
	print1 $r14fmt, 8(%rsp)
	print1 $r15fmt, 0(%rsp)
	print1 $rbpfmt, %rbp
	print1 $rspfmt, %rsp
	print1 $csfmt, %cs
	print1 $dsfmt, %ds
	print1 $esfmt, %es
	print1 $fsfmt, %fs
	print1 $gsfmt, %gs

	pop %r15
	pop %r14
	pop %r13
	pop %r12
	pop %r11
	pop %r10
	pop %r9
	pop %r8
	pop %rsi
	pop %rdi
	pop %rdx
	pop %rcx
	pop %rbx
	pop %rax
	1: hlt; jmp 1b
	iretq

.global isr_14
.type isr_14, @function
isr_14:
	cli
	push %rax
	movq 16(%rsp), %rax
	mov %rax, (addr14)
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
	sti
	iretq

// .global isr_33
// .type isr_33, @function
// isr_33:
// 	cli
// 	push %rax

// 	inb $0x60, %al
// 	movb %al, (last_scancode)

// 	// Send EOI
// 	movb $0x61, %al
// 	outb %al, $0x20

// 	pop %rax
// 	sti
// 	iretq

.global isr_33
.type isr_33, @function
isr_33:
	cli
	pushall
	push $0
	call irq1
	add $8, %rsp
	popall
	sti
	iretq
