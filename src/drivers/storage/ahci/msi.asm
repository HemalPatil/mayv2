[bits 64]

section .text
	extern ahciMsiHandler
	global ahciMsiHandlerWrapper
ahciMsiHandlerWrapper:
	pushfq	; Save flags and align stack to 16-byte boundary
	cld
	call ahciMsiHandler
	popfq
	iretq
