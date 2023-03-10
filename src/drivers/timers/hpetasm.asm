[bits 64]

section .text
	extern hpetHandler
	extern terminalPrintString
	global hpetHandlerWrapper
hpetHandlerWrapper:
	fxsave64 [rsp + 56]		; 40 bytes of IRQ stack frame + 16-byte offset into InterruptDataZone
	push rax	; Save all general registers, SSE registers, and align stack to 16-byte boundary
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
	cld
	call hpetHandler
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
	fxrstor64 [rsp + 56]
	iretq
