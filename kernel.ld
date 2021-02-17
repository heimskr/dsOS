ENTRY(_start)

/* Tell where the various sections of the object files will be put in the final kernel image. */
SECTIONS {
	. = 4M;

	/* Multiboot header and .text section. */
	.text BLOCK(4K) : ALIGN(4K) {
		*(.multiboot)
		*(.text)
		. = ALIGN(8);
		_ctors_start = .;
		KEEP(*(.init_array .ctors))
		_ctors_end = .;
	}

	/* Read-only data. */
	.rodata BLOCK(4K) : ALIGN(4K) {
		*(.rodata)
	}

	/* Read-write data (initialized) */
	.data BLOCK(4K) : ALIGN(4K) {
		*(.data)
	}

	/* Read-write data (uninitialized) and stack */
	.bss BLOCK(4K) : ALIGN(4K) {
		*(COMMON)
		*(.bss*)
	}

	_kernel_virtual_end = .;

	_kernel_physical_end = (LOADADDR(.bss) + SIZEOF(.bss) + 0xFFF) & 0xFFFFFFFFFFFFF000;
}
