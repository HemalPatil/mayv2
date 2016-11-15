[bits 64]

; IDT for 64 bit mode

IST1_STACK_SIZE equ 4096 ; 4 KiB stack
IST2_STACK_SIZE equ 4096 ; 4 KiB stack

; IST stack 1 - custom interrupt handlers
; IST stack 2 - processor exception handlers
;Reserve space for IST stacks
section .ISTs
	global IST1_stack_end
	global IST2_stack_end
IST1_stack:
	times IST1_STACK_SIZE db 0
IST1_stack_end:
IST2_stack:
	times IST2_STACK_SIZE db 0
IST2_stack_end:

section .IDT64
	global __IDT_START
	global __IDT_END

__IDT_START:
	times 224 - ($-$$) db 0		; Skip first 14 interrupts/exception

PageFaultDescriptor:
	dw 0	; offset[0..15]
	dw 0x8	; 64 bit code segment selector
	dw 0x8e01	; descriptor type and IST index
	dw 0	; offset[16..31]
	dq 0	; offset[32..63] and reserved

	times 512 - ($- $$) db 0	; Skip first 32 (0x00 to 0x1f) interrupts

Interrupt32Descriptor:
	dw 0	; offset[0..15]
	dw 0x8	; 64 bit code segment selector
	dw 0x8e01	; descriptor type and IST index
	dw 0	; offset[16..31]
	dq 0	; offset[32..63] and reserved

	times 4096 - ($-$$) db 0	; Make the 64-bit IDT 4 KiB long
__IDT_END:

section .rodata:
	PageFaultString db 10, 'Page Fault Exception', 10, 0, 0

section .text
	extern TerminalPrintString
	global PopulateIDTWithOffsets
PopulateIDTWithOffsets:
	mov rdx, Interrupt32Descriptor
	mov rax, Interrupt32Handler
	call FillOffsets
	mov rdx, PageFaultDescriptor
	mov rax, PageFaultHandler
	call FillOffsets
	ret

FillOffsets:
	mov [rdx], ax
	shr rax, 16
	mov [rdx + 6], ax
	shr rax, 16
	mov [rdx + 8], eax
	ret

PageFaultHandler:
	mov r8, 0xdeadbeefdeadc0de
	mov rdi, PageFaultString
	mov rsi, 22
	call TerminalPrintString
	hlt
	iretq

Interrupt32Handler:
	mov r8, 0xdeadc0de
	hlt