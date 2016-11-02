ENTRY(kernel_start);

SECTIONS
{
	. = 0x80000000;
	.lowerhalf : ALIGN(0x1000)
	{
		*(.lowerhalf);
	}

	. = 0xffffffff80000000;
	__IDT_START = .;
	.IDT : ALIGN(0x1000)
	{
		*(.IDT);
	}
	__IDT_END = .;
	.text : ALIGN(0x1000)
	{
		*(.text);
	}
	.data : ALIGN(0x1000)
	{
		*(.data);
	}
	.rodata : ALIGN(0x1000)
	{
		*(.rodata*);
	}
	.bss : ALIGN(0x1000)
	{
		*(COMMON);
		*(.bss);
	}
}