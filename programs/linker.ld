ENTRY (_start)

SECTIONS
{
	. = 0x500000;
	.text ALIGN(4K) : { *(.text) }
	.data ALIGN(4K)  : { *(.data) }
	.bss  ALIGN(4K) : { *(.bss) }
	.note.GNU-stack : {
	}
}
