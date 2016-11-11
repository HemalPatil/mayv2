[bits 64]

; IDT for 64 bit mode

IST1_STACK_SIZE equ 4096 ; 64 KiB stack

;Reserve space for IST1 stack
section .IST1STACK
	global IST1_stack_end
IST1_stack:
	times IST1_STACK_SIZE db 0
IST1_stack_end:

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
