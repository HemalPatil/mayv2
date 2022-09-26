[bits 64]

section .rodata:
	ahciStr db 'ahci', 0

section .text
	extern acknowledgeLocalApicInterrupt
	extern ahciMsiHandler
	extern terminalPrintString
	global ahciMsiHandlerWrapper
ahciMsiHandlerWrapper:
	cld
	mov rdi, ahciStr
	mov rsi, 4
	call terminalPrintString
	call ahciMsiHandler
	iretq