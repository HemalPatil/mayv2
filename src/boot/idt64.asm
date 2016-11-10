[bits 64]

; IDT for 64 bit mode

section .IDT64
	global __IDT_START
	global __IDT_END
__IDT_START:
IRQ0descriptor:
	dw 0	; offset 0..15
	dw 0x30	; 64-bit code segment
	
	times 4096 - ($-$$) db 0	; Make the 64-bit IDT 4 KiB long
__IDT_END:

section .text
	global IRQ0handler
IRQ0handler:
