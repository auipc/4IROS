ENTRY (kx86_64_preinit)

SECTIONS
{
	BASE = 0xffffff8000103000;
	. = 0x00100000;
	_kernel_start = .;
	. += BASE;

	.text ALIGN (4K) : AT (ADDR (.text) - BASE)
	{
		*(.text)
	}

	.rodata ALIGN (4K) : AT (ADDR (.rodata) - BASE)
	{
		*(.rodata)
	}

	.data ALIGN (4K) : AT (ADDR (.data) - BASE)
	{
		*(.data)
	}

	.bss ALIGN (4K) : AT (ADDR (.bss) - BASE)
	{
		*(COMMON)
		*(.bss)
		*(.bootstrap_stack)
	}

	.temp_page ALIGN(4K) : {
		_heap_start = .;
		. += 0x80000;
		_heap_end = .;
	}

	.note.GNU-stack : {
	}
	_kernel_end = .;
}
