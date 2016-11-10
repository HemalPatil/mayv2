[bits 64]

; TSS for 64 bit mode

section .TSS64
	global __TSS_START
	global __TSS_END
__TSS_START:
	dq 0
	dq 0
	dq 0
	dq 0
	dd 0
	dq 0	; IST1 entry (Interrupt Stack Table)
	dq 0
	dq 0
	dq 0
	dq 0
	dq 0
	dq 0
	dq 0
	dd 0
__TSS_END: