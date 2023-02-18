[bits 64]

section .text
	extern floatSaveRegion
	extern ps2KeyboardHandler
	extern terminalPrintString
	global ps2KeyboardHandlerWrapper
ps2KeyboardHandlerWrapper:
	pushfq	; Save all general registers, SSE registers, and flags
	push rax
	push rbx
	push rcx
	push rdx
	push rsi
	push rdi
	push r8
	push r9
	push r10
	push r11
	push r12
	push r13
	push r14
	push r15
	push rbp
	push rbp	; Required to align stack to 16-byte boundary
	fxsave64 [floatSaveRegion]
	in al, 0x60
	call ps2KeyboardHandler
	fxrstor64 [floatSaveRegion]
	pop rbp
	pop rbp
	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	pop r9
	pop r8
	pop rdi
	pop rsi
	pop rdx
	pop rcx
	pop rbx
	pop rax
	popfq
	iretq
