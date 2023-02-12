[bits 64]

section .text
	extern hpetHandler
	extern terminalPrintString
	global hpetHandlerWrapper
hpetHandlerWrapper:
	pushfq	; Save flags and align stack to 16-byte boundary
	cld
	call hpetHandler
	popfq
	iretq
