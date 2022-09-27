[bits 64]

section .text
	extern keyboardHandler
	extern terminalPrintString
	global keyboardHandlerWrapper
keyboardHandlerWrapper:
	in al, 0x60
	call keyboardHandler
	iretq
