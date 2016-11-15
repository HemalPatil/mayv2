[bits 64]

; TSS for 64 bit mode

section .TSS64
	extern setup_tss
	extern __GDT_START
	extern __GDT_END
	extern __IDT_START
	extern __IDT_END
	extern IST1_stack_end
	extern IST2_stack_end
	global __TSS_START
	global __TSS_END
	global GDTDescriptor
	global IDTDescriptor
	global NewGDTFarJump
__TSS_START:
reserved	dd 0
rsp0		dq 0
rsp1		dq 0
rsp2		dq 0
reserved1	dq 0
IST1		dq IST1_stack_end
IST2		dq IST2_stack_end
IST3		dq 0
IST4		dq 0
IST5		dq 0
IST6		dq 0
IST7		dq 0
reserved2	dq 0
reserved3	dw 0
iomapbase	dw 0
__TSS_END:

	; to save space in other sections we are declaring these variables here
GDTDescriptor:
	GDT64Limit dw 65535
	GDT64Base dq __GDT_START

IDTDescriptor:
	IDT64Limit dw 4095
	IDT64Base dq __IDT_START