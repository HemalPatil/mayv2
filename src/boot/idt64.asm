[bits 64]

; IDT for 64 bit mode

IST1_STACK_SIZE equ 4096	; 4 KiB stack
IST2_STACK_SIZE equ 4096	; 4 KiB stack

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
	times 128 - ($-$$) db 0		; Skip first 8 interrupts/exception

DoubleFaultDescriptor:
	dq 0
	dq 0

	times 224 - ($-$$) db 0		; Skip first 14 interrupts/exception

PageFaultDescriptor:
	dq 0
	dq 0

	times 4096 - ($-$$) db 0	; Make the 64-bit IDT 4 KiB long
__IDT_END:

section .rodata:
IDTDescriptor:
	IDT64Limit dw 4095
	IDT64Base dq __IDT_START
	DefaultInterruptString db 'Default interrupt handler!', 10, 0
	DoubleFaultString db 'Double Fault!', 10, 0
	IDTLoading db 10, 'Loading IDT...', 10, 0
	IDTLoaded db 'IDT loaded', 10, 0
	InterruptsEnabled db 'Enabled interrupts', 10, 0
	PageFaultString db 'Page Fault!', 10, 0

section .text
	extern TerminalPrintString
	global SetupIDT64
	global EnableInterrupts
SetupIDT64:
	mov rdi, IDTLoading
	mov rsi, 16
	call TerminalPrintString
; Fill all 256 interrupt handlers with DefaultInterruptHandler
; Set rdx to base address of IDT and loop through all 256
	mov r9, 0x00008e0200080000	; 64-bit code selector, descriptor type, and IST_2 index
	mov rdx, __IDT_START
SetupIDT64DescriptorLoop:
	mov [rdx], r9
	mov rax, DefaultInterruptHandler
	call FillOffsets
	add rdx, 16
	cmp rdx, __IDT_END
	jle SetupIDT64DescriptorLoop
	mov rdx, PageFaultDescriptor
	mov rax, PageFaultHandler
	call FillOffsets
	mov rdx, DoubleFaultDescriptor
	mov rax, DoubleFaultHandler
	call FillOffsets
	mov rdi, IDTLoaded
	mov rsi, 11
	call TerminalPrintString
	mov rax, IDTDescriptor
	lidt [rax]	; load the IDT
	ret

FillOffsets:
	mov [rdx], ax
	shr rax, 16
	mov [rdx + 6], ax
	shr rax, 16
	mov [rdx + 8], eax
	ret

EnableInterrupts:
	sti
	mov rdi, InterruptsEnabled
	mov rsi, 19
	call TerminalPrintString
	ret

DoubleFaultHandler:
	mov rdi, DoubleFaultString
	mov rsi, 14
	call TerminalPrintString
	pop r8	; Pop the 64 bit error code in thrashable register
	iretq

PageFaultHandler:
	mov rdi, PageFaultString
	mov rsi, 12
	call TerminalPrintString
	pop r8	; Pop the 32 bit error code in thrashable register
	iretq

DefaultInterruptHandler:
	mov rdi, DefaultInterruptString
	mov rsi, 27
	call TerminalPrintString
	pop r8	; Pop the 64 bit error code in thrashable register
	iretq