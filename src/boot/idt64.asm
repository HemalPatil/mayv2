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
	times 512 - ($- $$) db 0	; Skip first 32 (0x00 to 0x1f) interrupts

Interrupt32Descriptor:
	dw 0	; offset[0..15]
	dw 0x8	; 64 bit code segment selector
	dw 0x8e01	; descriptor type and IST index
	dw 0	; offset[16..31]
	dq 0	; offset[32..63] and reserved

	times 4096 - ($-$$) db 0	; Make the 64-bit IDT 4 KiB long
__IDT_END:

section .text
	global PopulateIDTWithOffsets
PopulateIDTWithOffsets:
	mov rdx, Interrupt32Descriptor
	mov rax, Interrupt32Handler
	mov [rdx], ax
	shr rax, 16
	mov [rdx + 6], ax
	shr rax, 16
	mov [rdx + 8], eax
	ret

Interrupt32Handler:
	hlt
