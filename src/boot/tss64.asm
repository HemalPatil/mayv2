[bits 64]

; TSS for 64 bit mode

section .TSS64
	extern IDT_START
	extern IDT_END
	extern IST1_STACK_END
	extern IST2_STACK_END
TSS_START:
reserved	dd 0
rsp0		dq 0
rsp1		dq 0
rsp2		dq 0
reserved1	dq 0
IST1		dq IST1_STACK_END
IST2		dq IST2_STACK_END
IST3		dq 0
IST4		dq 0
IST5		dq 0
IST6		dq 0
IST7		dq 0
reserved2	dq 0
reserved3	dw 0
iomapbase	dw 0
TSS_END:

section .rodata
	tssLoading db 10, 'Loading TSS...', 10, 0
	tssLoaded db 'TSS loaded', 10, 0

section .text
	extern GDT_START
	extern terminalPrintString
	global setupTss64
setupTss64:
	mov rdi, tssLoading
	mov rsi, 16
	call terminalPrintString
	mov rdx, GDT_START
	add rdx, 0x1a	; point to TSS descriptor byte 2
	mov rax, TSS_START
	mov [rdx], ax	; move base[0..16] of TSS
	shr rax, 16
	mov [rdx + 2], al	; move base[16..23] of TSS
	shr rax, 8
	mov [rdx + 5], al	; move base[24..31] of TSS 
	shr rax, 8
	mov [rdx + 6], eax	; move base[32..63] of TSS
	mov ax, 0x18
	ltr ax
	mov rdi, tssLoaded
	mov rsi, 11
	call terminalPrintString
	ret