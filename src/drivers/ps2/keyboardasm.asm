[bits 64]

section .text
	extern ps2KeyboardHandler
	extern terminalPrintString
	global ps2KeyboardHandlerWrapper
ps2KeyboardHandlerWrapper:
	in al, 0x60
	call ps2KeyboardHandler
	iretq
