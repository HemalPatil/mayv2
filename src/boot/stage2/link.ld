ENTRY(kernel32start);

SECTIONS
{
	. = 0x100000;

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