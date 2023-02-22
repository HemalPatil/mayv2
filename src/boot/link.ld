ENTRY(kernelCompatibilityModeStart)

PHDRS {
	lowerhalf PT_LOAD;
	text PT_LOAD;
	rodata PT_LOAD;
	data PT_LOAD;
}

SECTIONS {
	. = 0x80000000;
	.lowerhalf : {
		*(.lowerhalf)
	} : lowerhalf

	. = 0xffffffff80000000;
	.text : {
		*(.text)
		*(.text.*)
	} : text

	. = ALIGN(0x1000);
	.rodata : {
		*(.rodata)
		*(.rodata.*)
	} : rodata

	.eh_frame_hdr : {
		*(.eh_frame_hdr)
	}
	.eh_frame : {
		*(.eh_frame)
		*(.eh_frame.*)
	}

	. = ALIGN(0x1000);
	.data : {
		*(.data)
		*(.data.*)
	} : data
	.bss : {
		*(.bss)
		*(.bss.*)
		*(COMMON)
	}
}
