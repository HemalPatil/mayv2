[bits 64]

section .text
	extern ahciMsiHandler
	global ahciMsiHandlerWrapper
ahciMsiHandlerWrapper:
	; pushfq
	; push rbp
	; cld
	call ahciMsiHandler
	; pop rbp
	; popfq
	; cli
	; hlt
	iretq
