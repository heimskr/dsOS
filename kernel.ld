ENTRY(_start)

SECTIONS {
	. = 4M;
	_kernel_physical_start = .;

	.boottext : {
		asm/boot.o(.multiboot)
		asm/boot.o(.text)
		/* src/init.o(.text) */
	}

	.bootrodata : {
		asm/boot.o(.rodata)
	}

	.bootdata : {
		asm/boot.o(.data)
	}

	.bootbss : {
		asm/boot.o(.bss)
		asm/boot.o(COMMON)
	}

	. = ALIGN(8);
	_bitmap_start = .;
	. += 8388608;
	_bitmap_end = .;

	. = ALIGN(0x1000);
	_boot_end = .;

	/* . += 0xFFFFFFFF80000000; */
	_kernel_virtual_start = .;

	.text : {
		/* *(.multiboot) */
		*(.text)
		*(.text.*)
		. = ALIGN(8);
		_ctors_start = .;
		KEEP(*(.init_array .ctors))
		_ctors_end = .;
	}

	. = ALIGN(0x1000);

	.rodata : AT ( (LOADADDR (.text) + SIZEOF (.text) + 0xFFF) & 0xFFFFFFFFFFFFF000 ) {
		*(.rodata)
		*(.rodata.*)
	}

	. = ALIGN(0x1000);

	.data : AT ( (LOADADDR (.rodata) + SIZEOF (.rodata) + 0xFFF) & 0xFFFFFFFFFFFFF000 ) {
		*(.data)
	}

	. = ALIGN(0x1000);

	.bss : AT ( (LOADADDR (.data) + SIZEOF (.data) + 0xFFF) & 0xFFFFFFFFFFFFF000 ) {
		*(COMMON)
		*(.bss)
	}

	. += (10 << 20);
	. = ALIGN(0x1000);

	_kernel_virtual_end = .;

	_kernel_physical_end = (LOADADDR (.bss) + SIZEOF (.bss) + 0xFFF) & 0xFFFFFFFFFFFFF000;
}