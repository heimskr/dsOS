ENTRY(_start)

SECTIONS {
	. = 4M;
	_kernel_physical_start = .;

	.boottext : {
		asm/boot.o(.multiboot)
		asm/boot.o(.text)
	}

	.bootrodata : {
		asm/boot.o(.rodata)
	}

	.bootdata : {
		asm/boot.o(.data)
		. = ALIGN(4);
		_bitmap_start = .;
		. += 8388608;
		_bitmap_end = .;
	}

	.bootbss : {
		asm/boot.o(.bss)
		asm/boot.o(COMMON)
	}

	. = ALIGN(0x1000);

	. += 0xFFFFFFFF80000000;
	_kernel_virtual_start = .;

	.text : AT(_boot_end) {
		*(.multiboot)
		*(.text)
		. = ALIGN(8);
		_ctors_start = .;
		KEEP(*(.init_array .ctors))
		_ctors_end = .;
	}

	.rodata : ALIGN(4K) {
		*(.rodata)
	}

	.data : ALIGN(4K) {
		*(.data)
	}

	.bss : ALIGN(4K) {
		*(COMMON)
		*(.bss)
	}

	_kernel_virtual_end = .;

	_kernel_physical_end = (LOADADDR (.bss) + SIZEOF (.bss) + 0xFFF) & 0xFFFFFFFFFFFFF000;
}
