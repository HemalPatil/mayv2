ENTRY(kernelStart);

SECTIONS {
	. = 0x80000000;
	.lowerhalf : ALIGN(0x1000) {
		*(.lowerhalf);
	}

	. = 0xffffffff80000000;
	.GDT64 : ALIGN(0x1000) {
		*(.GDT64);
	}
	.IDT64 : ALIGN(0x1000) {
		*(.IDT64);
	}
	.TSS64 : ALIGN(0x1000) {
		*(.TSS64);
	}
	.KERNELSTACK : ALIGN(0x1000) {
		*(.KERNELSTACK);
	}
	.ISTs : ALIGN(0x1000) {
		*(.ISTs);
	}
	.text : ALIGN(0x1000) {
		*(.text);
	}
	.data : ALIGN(0x1000) {
		*(.data);
	}
	.rodata : ALIGN(0x1000) {
		*(.rodata*);
	}
	.bss : ALIGN(0x1000) {
		*(COMMON);
		*(.bss);
	}
}