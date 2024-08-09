ENTRY (kx86_64_preinit)

SECTIONS
{
	. = 0x00100000;
	_kernel_start = .;
	. += 0x280000000;

	.text ALIGN (4K) : AT (ADDR (.text) - 0x280000000)
	{
		*(.text)
	}

	.rodata ALIGN (4K) : AT (ADDR (.rodata) - 0x280000000)
	{
		*(.rodata)
	}

	.data ALIGN (4K) : AT (ADDR (.data) - 0x280000000)
	{
		*(.data)
	}

	.bss ALIGN (4K) : AT (ADDR (.bss) - 0x280000000)
	{
		*(COMMON)
		*(.bss)
		*(.bootstrap_stack)
	}

	_kernel_end = .;
}
