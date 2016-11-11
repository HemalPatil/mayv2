[bits 64]

; TSS for 64 bit mode

section .TSS64
	extern setup_tss
	extern __GDT_START
	extern __GDT_END
	extern __IDT_START
	extern __IDT_END
	global __TSS_START
	global __TSS_END
	global GDTDescriptor
	global IDTDescriptor
	global NewGDTFarJump
__TSS_START:
	dq 0
	dq 0
	dq 0
	dq 0
	dd 0
	dq 0	; IST1 entry (Interrupt Stack Table)
	dq 0
	dq 0
	dq 0
	dq 0
	dq 0
	dq 0
	dq 0
	dd 0
__TSS_END:

	; to save space in other sections we are declaring these variables here
GDTDescriptor:
	GDT64Limit dw 65535
	GDT64Base dq __GDT_START

IDTDescriptor:
	IDT64Limit dw 4095
	IDT64Base dq __IDT_START