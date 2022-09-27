[bits 64]

; IDT for 64 bit mode

IST1_STACK_SIZE equ 4096	; 4 KiB stack
IST2_STACK_SIZE equ 4096	; 4 KiB stack

; IST stack 1 - custom interrupt handlers
; IST stack 2 - processor exception handlers
; Reserve space for IST stacks
section .ISTs
	global IST1_STACK_END
	global IST2_STACK_END
IST1_STACK:
	times IST1_STACK_SIZE db 0
IST1_STACK_END:
IST2_STACK:
	times IST2_STACK_SIZE db 0
IST2_STACK_END:

section .IDT64
	global IDT_START
	global IDT_END

IDT_START:
	times 96 - ($-$$) db 0		; Skip first 6 interrupts/exception

invalidOpcodeDescriptor:
	dq 0
	dq 0

	times 128 - ($-$$) db 0		; Skip first 8 interrupts/exception

doubleFaultDescriptor:
	dq 0
	dq 0

	times 224 - ($-$$) db 0		; Skip first 14 interrupts/exception

pageFaultDescriptor:
	dq 0
	dq 0

	times 4096 - ($-$$) db 0	; Make the 64-bit IDT 4 KiB long
IDT_END:

section .rodata:
idtDescriptor:
	idt64Limit dw 4095
	idt64Base dq IDT_START
	defaultInterruptStr db 'Default interrupt handler!', 10, 0
	doubleFaultStr db 'Double Fault!', 10, 0
	idtLoadingStr db 'Loading IDT', 0
	interruptsEnabledStr db 'Enabled interrupts', 10, 0
	pageFaultStr db 'Page Fault! Tried to access ', 0
	invalidOpcodeStr db 'Invalid opcode', 0
	invalidInterruptStr db 'Invalid interrupt number [', 0

section .text
	extern doneStr
	extern ellipsisStr
	extern acknowledgeLocalApicInterrupt
	extern hangSystem
	extern terminalPrintChar
	extern terminalPrintDecimal
	extern terminalPrintHex
	extern terminalPrintString
	global enableInterrupts
	global installIdt64Entry
	global setupIdt64
setupIdt64:
	mov rdi, idtLoadingStr
	mov rsi, 11
	call terminalPrintString
	mov rdi, [ellipsisStr]
	mov rsi, 3
	call terminalPrintString
; Fill all 256 interrupt handlers with defaultInterruptHandler
; Set rdx to base address of IDT and loop through all 256
	mov r9, 0x00008e0200080000	; 64-bit code selector, descriptor type, and IST_2 index
	mov rdx, IDT_START
setupIdt64DescriptorLoop:
	mov [rdx], r9
	mov rax, defaultInterruptHandler
	call fillOffsets
	add rdx, 16
	cmp rdx, IDT_END
	jle setupIdt64DescriptorLoop
	mov rdx, pageFaultDescriptor
	mov rax, pageFaultHandler
	call fillOffsets
	mov rdx, doubleFaultDescriptor
	mov rax, doubleFaultHandler
	call fillOffsets
	mov rdx, invalidOpcodeDescriptor
	mov rax, invalidOpcodeHandler
	call fillOffsets
	mov rax, idtDescriptor
	lidt [rax]	; load the IDT
	mov rdi, [doneStr]
	mov rsi, 4
	call terminalPrintString
	mov rdi, 10
	call terminalPrintChar
	ret

fillOffsets:
	mov [rdx], ax
	shr rax, 16
	mov [rdx + 6], ax
	shr rax, 16
	mov [rdx + 8], eax
	ret

installIdt64Entry:
	cmp rdi, 255
	jge installIdt64EntryInvalidInterrupt
	cmp rdi, 32
	jl installIdt64EntryInvalidInterrupt
	mov rax, rdi
	shl rax, 4
	mov rdx, IDT_START
	add rdx, rax
	mov rax, rsi
	call fillOffsets
	ret
installIdt64EntryInvalidInterrupt:
	push rdi
	mov rdi, invalidInterruptStr
	mov rsi, 26
	call terminalPrintString
	pop rdi
	call terminalPrintDecimal
	mov rdi, ']'
	call terminalPrintChar
	mov rdi, 1
	call hangSystem
	ret

enableInterrupts:
	sti
	mov rdi, interruptsEnabledStr
	mov rsi, 19
	call terminalPrintString
	ret

doubleFaultHandler:
	mov rdi, doubleFaultStr
	mov rsi, 14
	call terminalPrintString
	pop r8	; Pop the 64 bit error code in thrashable register
	iretq

pageFaultHandler:
	mov rdi, pageFaultStr
	mov rsi, 28
	call terminalPrintString

	; Pop the 32 bit error code in thrashable register
	pop r8

	; Get the virtual address that caused this fault
	; and display it
	mov rax, cr2
	push rax
	mov rdi, rsp
	mov rsi, 8
	call terminalPrintHex
	pop rax
	mov rdi, 10
	call terminalPrintChar
	iretq

defaultInterruptHandler:
	mov rdi, defaultInterruptStr
	mov rsi, 27
	call terminalPrintString
	pop r8	; Pop the 64 bit error code in thrashable register
	iretq

invalidOpcodeHandler:
	mov rdi, invalidOpcodeStr
	mov rsi, 14
	call terminalPrintString
	; TODO: better recovery from invalid opcode
	cli
	hlt
	pop r8	; Pop the 64 bit error code in thrashable register
	iretq
