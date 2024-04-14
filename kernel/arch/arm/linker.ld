ENTRY (_start)

SECTIONS
{
	. = 0x80000;

	_kernel_start = .;

	.text : ALIGN(0x1000) {
		*(.text)
	}

	.data : {
		*(.data)
	}

	_boot_shim_end = .;
}
