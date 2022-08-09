[bits 64]

; TSS for 64 bit mode

section .TSS64
	extern __IDT_START
	extern __IDT_END
	extern IST1_stack_end
	extern IST2_stack_end
__TSS_START:
reserved	dd 0
rsp0		dq 0
rsp1		dq 0
rsp2		dq 0
reserved1	dq 0
IST1		dq IST1_stack_end
IST2		dq IST2_stack_end
IST3		dq 0
IST4		dq 0
IST5		dq 0
IST6		dq 0
IST7		dq 0
reserved2	dq 0
reserved3	dw 0
iomapbase	dw 0
__TSS_END:

section .rodata
	TSSLoading db 10, 'Loading TSS...', 10, 0
	TSSLoaded db 'TSS loaded', 10, 0

section .text
	extern __GDT_START
	extern TerminalPrintString
	global SetupTSS64
SetupTSS64:
	mov rdi, TSSLoading
	mov rsi, 16
	call TerminalPrintString
	mov rdx, __GDT_START
	add rdx, 0x1a	; point to TSS descriptor byte 2
	mov rax, __TSS_START
	mov [rdx], ax	; move base[0..16] of TSS
	shr rax, 16
	mov [rdx + 2], al	; move base[16..23] of TSS
	shr rax, 8
	mov [rdx + 5], al	; move base[24..31] of TSS 
	shr rax, 8
	mov [rdx + 6], eax	; move base[32..63] of TSS
	mov ax, 0x18
	ltr ax
	mov rdi, TSSLoaded
	mov rsi, 11
	call TerminalPrintString
	ret