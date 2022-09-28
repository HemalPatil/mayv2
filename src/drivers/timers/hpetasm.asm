[bits 64]

section .text
	extern hpetHandler
	extern terminalPrintString
	global hpetHandlerWrapper
hpetHandlerWrapper:
	call hpetHandler
	iretq
