# Based on code by Ring Zero and Lower: http://ringzeroandlower.com/2017/08/08/x86-64-kernel-boot.html

#include "arch/x86_64/gdt.h"
#include "arch/x86_64/mmu.h"
#include "kernel_core.h"
#include "sizes.h"
#include "multiboot2.h"
#include "arch/x86_64/msr.h"


.SET HEADER_LENGTH, header_end - header_start
.SET CHECKSUM, -(MULTIBOOT2_HEADER_MAGIC + MULTIBOOT_ARCHITECTURE_I386 + HEADER_LENGTH)
.section .multiboot
header_start:
    .long MULTIBOOT2_HEADER_MAGIC
    .long MULTIBOOT_ARCHITECTURE_I386
    .long HEADER_LENGTH
    .long CHECKSUM

    # multiboot tags go here.

    .short MULTIBOOT_HEADER_TAG_END
    .short 0 # flags, none set
    .long 8  # size, including itself (short + short + long)
header_end:

.code32

.section .bss
.comm pml4, PML4_SIZE, PML4_ALIGNMENT
.comm tmp_stack, KERNEL_BOOT_STACK_SIZE, KERNEL_BOOT_STACK_ALIGNMENT
.comm multiboot_magic, 4
.comm multiboot_data, 8
.comm __dso_handle, 8

.section text
#include "../32/paging.S"

.data
.align GDT_TABLE_ALIGNMENT
gdt_table:
	.8byte GDT_FIRST_ENTRY
	.8byte GDT_KERNEL_CODE_ENTRY
	.8byte GDT_KERNEL_DATA_ENTRY

gdt_table_end:
	.skip (GDT_TABLE_SIZE - (gdt_table_end - gdt_table))

gdt_ptr:
	.short GDT_TABLE_SIZE - 1
	.long gdt_table

.section .text
.global _start
.type _start, @function
_start:
	movl %eax, multiboot_magic
	movl %ebx, multiboot_data
	movl $tmp_stack + KERNEL_BOOT_STACK_SIZE, %esp

	// PIC
	mov $0xff, %al
	outb %al, $0xa1
	outb %al, $0x21

	call setup_paging

	movl $pml4, %eax
	movl %eax, %cr3

	movl $KERNEL_CR4, %eax
	movl %eax, %cr4

	// Set the long mode enable bit in the EFER.
	movl $MSR_EFER, %ecx
	rdmsr
	or $MSR_EFER_LME, %eax
	wrmsr

	movl $KERNEL_CR0, %eax
	movl %eax, %cr0

	lgdt gdt_ptr

	ljmp $(KERNEL_GDT_ENTRY * GDT_ENTRY_SIZE), $_start64

	cli
	hlt

.code64

.global _start64
.type _start64, @function
_start64:
	// Set up segment selectors
	xor %rax, %rax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %fs
	movw %ax, %gs
	movw %ax, %ss

	movabs $_kernel_virtual_end, %rsp
	movq %rsp, %rbp

	callq kernel_main

	// Should never reach here
	cli
1:
	hlt
	jmp 1b
