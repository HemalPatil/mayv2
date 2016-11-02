[bits 64]

; Interrupt Tables and functions

section .IDT
	global InterruptDescriptorTable
InterruptDescriptorTable:
	times 65536 - ($-$$) db 0	; Make the IDT 64 KiB long