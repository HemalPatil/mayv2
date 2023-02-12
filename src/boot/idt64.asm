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
	doubleFaultStr db 'Double fault', 0
	gpFaultStr db 'General protection fault at ', 0
	pageFaultStr1 db 'Page fault at ', 0
	pageFaultStr2 db ' tried to access ', 0
	withErrorStr db ' with error ', 0
	idtLoadingStr db 'Loading IDT', 0
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
; FIXME: fills all 256 interrupt handlers with present flags
; Set rdx to base address of IDT and loop through all 256
	mov r9, 0x00008e0200080000	; 64-bit code selector, descriptor type, and IST_2 index
	mov rdx, IDT_START
setupIdt64DescriptorLoop:
	mov [rdx], r9
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
	mov rdx, IDT_START + 13 * 16
	mov rax, gpFaultHandler
	call fillOffsets
	add rdx, 16
	mov rax, gpFaultHandler
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
	push rdi	; Align stack to 16-byte boundary
	mov rdi, 10
	call terminalPrintChar
	mov rdi, divisionByZeroStr
	mov rsi, 20
	call terminalPrintString
	mov rdi, rsp
	mov rsi, 8
	add rdi, rsi
	call terminalPrintHex
	pop rdi
	cli
	hlt
	iretq

debugHandler:
	push rdi	; Align stack to 16-byte boundary
	mov rdi, 10
	call terminalPrintChar
	mov rdi, debugStr
	mov rsi, 19
	call terminalPrintString
	mov rdi, rsp
	mov rsi, 8
	add rdi, rsi
	call terminalPrintHex
	pop rdi
	cli
	hlt
	iretq

nmiHandler:
	push rdi	; Align stack to 16-byte boundary
	mov rdi, 10
	call terminalPrintChar
	mov rdi, nmiHandlerStr
	mov rsi, 11
	call terminalPrintString
	pop rdi
	cli
	hlt
	iretq

breakpointHandler:
	push rdi	; Align stack to 16-byte boundary
	mov rdi, 10
	call terminalPrintChar
	mov rdi, breakpointStr
	mov rsi, 24
	call terminalPrintString
	mov rdi, rsp
	mov rsi, 8
	add rdi, rsi
	call terminalPrintHex
	pop rdi
	cli
	hlt
	iretq

overflowHandler:
	push rdi	; Align stack to 16-byte boundary
	mov rdi, 10
	call terminalPrintChar
	mov rdi, overflowStr
	mov rsi, 22
	call terminalPrintString
	mov rdi, rsp
	mov rsi, 8
	add rdi, rsi
	call terminalPrintHex
	pop rdi
	cli
	hlt
	iretq

boundRangeHandler:
	push rdi	; Align stack to 16-byte boundary
	mov rdi, 10
	call terminalPrintChar
	mov rdi, boundRangeStr
	mov rsi, 24
	call terminalPrintString
	mov rdi, rsp
	mov rsi, 8
	add rdi, rsi
	call terminalPrintHex
	pop rdi
	cli
	hlt
	iretq

invalidOpcodeHandler:
	push rdi	; Align stack to 16-byte boundary
	mov rdi, 10
	call terminalPrintChar
	mov rdi, invalidOpcodeStr
	mov rsi, 18
	call terminalPrintString
	mov rdi, rsp
	mov rsi, 8
	add rdi, rsi
	call terminalPrintHex
	pop rdi
	cli
	hlt
	iretq

noSseHandler:
	push rdi	; Align stack to 16-byte boundary
	mov rdi, 10
	call terminalPrintChar
	mov rdi, noSseStr
	mov rsi, 27
	call terminalPrintString
	mov rdi, rsp
	mov rsi, 8
	add rdi, rsi
	call terminalPrintHex
	pop rdi
	cli
	hlt
	iretq

doubleFaultHandler:
	mov rdi, 10
	call terminalPrintChar
	mov rdi, doubleFaultStr
	mov rsi, 12
	call terminalPrintString
	pop r8	; Pop the 64 bit error code in thrashable register
	cli
	hlt
	iretq

gpFaultHandler:
	mov rdi, 10
	call terminalPrintChar
	mov rdi, gpFaultStr
	mov rsi, 28
	call terminalPrintString
	mov rdi, rsp
	mov rsi, 8
	add rdi, rsi
	call terminalPrintHex
	mov rdi, withErrorStr
	mov rsi, 12
	call terminalPrintString
	mov rdi, rsp
	mov rsi, 8
	call terminalPrintHex
	pop r8	; Pop the 64 bit error code in thrashable register
	cli
	hlt
	iretq

pageFaultHandler:
	mov rdi, 10
	call terminalPrintChar
	mov rdi, pageFaultStr1
	mov rsi, 14
	call terminalPrintString
	mov rdi, rsp
	mov rsi, 8
	add rdi, rsi
	call terminalPrintHex
	mov rdi, pageFaultStr2
	mov rsi, 17
	call terminalPrintString
	push rdi	; Align stack to 16-byte boundary
	; Get the virtual address that caused this fault and display it
	mov rax, cr2
	push rax
	mov rdi, rsp
	mov rsi, 8
	call terminalPrintHex
	pop rax
	pop rdi
	mov rdi, withErrorStr
	mov rsi, 12
	call terminalPrintString
	mov rdi, rsp
	mov rsi, 8
	call terminalPrintHex
	pop r8	; Pop the 64 bit error code in thrashable register
	cli
	hlt
	iretq
