[bits 64]

section .rodata:
	keyboardStr db 'key', 0

section .text
	extern acknowledgeLocalApicInterrupt
	extern terminalPrintString
	global keyboardHandler
keyboardHandler:
	in al, 0x60
	mov rdi, keyboardStr
	mov rsi, 3
	call terminalPrintString
	call acknowledgeLocalApicInterrupt
	iretq
