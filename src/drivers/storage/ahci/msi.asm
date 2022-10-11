[bits 64]

section .text
	extern ahciMsiHandler
	global ahciMsiHandlerWrapper
ahciMsiHandlerWrapper:
	cld
	call ahciMsiHandler
	iretq
