ENTRY(_start)

SECTIONS {
	. = 4M;
	_kernel_physical_start = .;

	.text : {
		asm/boot.o(.multiboot)
		asm/boot.o(.text)
		*(.text)
		*(.text.*)
		. = ALIGN(8);
		_ctors_start = .;
		KEEP(*(.init_array .ctors))
		_ctors_end = .;
	}

	.rodata : {
		*(.rodata)
		*(.rodata.*)
	}

	.data : {
		*(.data)
	}

	.bss : {
		*(.bss)
		*(COMMON)
	}

	. = ALIGN(8);
	_bitmap_start = .;
	. += 8388608;
	_bitmap_end = .;

	. = ALIGN(0x1000);
	_kernel_physical_end = .;
}