[bits 64]

; GDT for 64 bit mode

section .GDT64
	global __GDT_START
	global __GDT_END
__GDT_START:
; null entry
	dq 0

; 64 bit code segment descriptor
	dq 0x00209a0000000000

; 64 bit data segment descriptor
	dq 0x0020920000000000

; 64 bit TSS descriptor
	; TSS base address to be filled in at runtime
	dw 104	; TSS size
	dw 0	; TSS base[0..15]
	db 0	; TSS base[16..23]
	db 0x89	; TSS access flags
	dw 0x10	; TSS flags and limit[16..19] and base[24..31]
	dq 0	; TSS base[32..63] and reserved dword

	times 65536 - ($-$$) db 0	; Make the GDT 64 KiB long

__GDT_END: