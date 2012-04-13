ENTRY(init)
OUTPUT_FORMAT(elf32-i386)

SECTIONS {
	. = 0xC0000000;

	.text : {
		*(.text)
		*(.rodata*)
	}

	.data : {
		*(.data)
	}

	.bss : {
		_sbss = .;
		*(COMMON)
		*(.bss)
		_ebss = .;
	}

	end = .; _end = .; __end = .;
}
