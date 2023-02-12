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
	divisionByZeroStr db 'Division by zero at ', 0
	debugStr db 'Debug exception at ', 0
	nmiHandlerStr db 'NMI handler', 0
	breakpointStr db 'Breakpoint exception at ', 0
	overflowStr db 'Overflow exception at ', 0
	boundRangeStr db 'Bound range exceeded at ', 0
	invalidOpcodeStr db 'Invalid opcode at ', 0
	noSseStr db 'Attempted to access SSE at ', 0
	defaultInterruptStr db 'Default interrupt handler!', 10, 0
	doubleFaultStr db 10, 'Double Fault', 0
	idtLoadingStr db 'Loading IDT', 0
	pageFaultStr db 'Page Fault! Tried to access ', 0
	invalidInterruptStr db 'Invalid interrupt number [', 0

section .data
	global availableInterrupt
	availableInterrupt dq 0x20

section .text
	extern doneStr
	extern ellipsisStr
	extern acknowledgeLocalApicInterrupt
	extern hangSystem
	extern terminalPrintChar
	extern terminalPrintDecimal
	extern terminalPrintHex
	extern terminalPrintString
	global disableInterrupts
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
	jl setupIdt64DescriptorLoop
	mov rdx, IDT_START
	mov rax, divisionByZeroHandler
	call fillOffsets
	add rdx, 16
	mov rax, debugHandler
	call fillOffsets
	add rdx, 16
	mov rax, nmiHandler
	call fillOffsets
	add rdx, 16
	mov rax, breakpointHandler
	call fillOffsets
	add rdx, 16
	mov rax, overflowHandler
	call fillOffsets
	add rdx, 16
	mov rax, boundRangeHandler
	call fillOffsets
	add rdx, 16
	mov rax, invalidOpcodeHandler
	call fillOffsets
	add rdx, 16
	mov rax, noSseHandler
	call fillOffsets
	add rdx, 16
	mov rax, doubleFaultHandler
	call fillOffsets
	add rdx, 16
	; Skip the deprecated INT #9
	add rdx, 16
	mov rdx, pageFaultDescriptor
	mov rax, pageFaultHandler
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
	call hangSystem
	ret

disableInterrupts:
	cli
	ret

enableInterrupts:
	sti
	ret

divisionByZeroHandler:
	mov rdi, 10
	call terminalPrintChar
	mov rdi, divisionByZeroStr
	mov rsi, 20
	call terminalPrintString
	mov rdi, rsp
	mov rsi, 8
	call terminalPrintHex
	cli
	hlt
	iretq

debugHandler:
	mov rdi, 10
	call terminalPrintChar
	mov rdi, debugStr
	mov rsi, 19
	call terminalPrintString
	mov rdi, rsp
	mov rsi, 8
	call terminalPrintHex
	cli
	hlt
	iretq

nmiHandler:
	mov rdi, 10
	call terminalPrintChar
	mov rdi, nmiHandlerStr
	mov rsi, 11
	call terminalPrintString
	cli
	hlt
	iretq

breakpointHandler:
	mov rdi, 10
	call terminalPrintChar
	mov rdi, breakpointStr
	mov rsi, 24
	call terminalPrintString
	mov rdi, rsp
	mov rsi, 8
	call terminalPrintHex
	cli
	hlt
	iretq

overflowHandler:
	mov rdi, 10
	call terminalPrintChar
	mov rdi, overflowStr
	mov rsi, 22
	call terminalPrintString
	mov rdi, rsp
	mov rsi, 8
	call terminalPrintHex
	cli
	hlt
	iretq

boundRangeHandler:
	mov rdi, 10
	call terminalPrintChar
	mov rdi, boundRangeStr
	mov rsi, 24
	call terminalPrintString
	mov rdi, rsp
	mov rsi, 8
	call terminalPrintHex
	cli
	hlt
	iretq

invalidOpcodeHandler:
	mov rdi, 10
	call terminalPrintChar
	mov rdi, invalidOpcodeStr
	mov rsi, 18
	call terminalPrintString
	mov rdi, rsp
	mov rsi, 8
	call terminalPrintHex
	cli
	hlt
	iretq

noSseHandler:
	mov rdi, 10
	call terminalPrintChar
	mov rdi, noSseStr
	mov rsi, 27
	call terminalPrintString
	mov rdi, rsp
	mov rsi, 8
	call terminalPrintHex
	cli
	hlt
	iretq

doubleFaultHandler:
	mov rdi, 10
	call terminalPrintChar
	mov rdi, doubleFaultStr
	mov rsi, 13
	call terminalPrintString
	pop r8	; Pop the 64 bit error code in thrashable register
	cli
	hlt
	iretq

pageFaultHandler:
	mov rdi, pageFaultStr
	mov rsi, 28
	call terminalPrintString

	; Pop the 8-byte aligned 32-bit error code in thrashable register
	pop r8

	; Get the virtual address that caused this fault and display it
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
	cli
	hlt
	iretq
