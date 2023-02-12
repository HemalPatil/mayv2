[bits 64]

section .text
	extern ps2KeyboardHandler
	extern terminalPrintString
	global ps2KeyboardHandlerWrapper
ps2KeyboardHandlerWrapper:
	pushfq	; Save flags and align stack to 16-byte boundary
	in al, 0x60
	call ps2KeyboardHandler
	popfq
	iretq
