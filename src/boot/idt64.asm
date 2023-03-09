[bits 64]

section .bss align=16
IDT_START:
	resb 4096	; Make the 64-bit IDT 4 KiB long
IDT_END:

section .rodata align=16
	global idt64Base
	global idtDescriptor
idtDescriptor:
	idt64Limit dw IDT_END - IDT_START - 1
	idt64Base dq IDT_START
	divisionByZeroStr db 10, 'Division by zero at ', 0
	debugStr db 10, 'Debug exception at ', 0
	nmiHandlerStr db 10, 'NMI handler', 0
	breakpointStr db 10, 'Breakpoint exception at ', 0
	overflowStr db 10, 'Overflow exception at ', 0
	boundRangeStr db 10, 'Bound range exceeded at ', 0
	invalidOpcodeStr db 10, 'Invalid opcode at ', 0
	noSseStr db 10, 'Attempted to access SSE at ', 0
	doubleFaultStr db 10, 'Double fault', 0
	gpFaultStr db 10, 'General protection fault at ', 0
	pageFaultStr1 db 10, 'Page fault at ', 0
	pageFaultStr2 db ' tried to access ', 0
	withErrorStr db ' with error ', 0
	onCpuStr db ' on CPU [', 0

section .data align=16
	global availableInterrupt
	availableInterrupt dq 0x20

section .text
	extern doneStr
	extern ellipsisStr
	extern terminalPrintChar
	extern terminalPrintDecimal
	extern terminalPrintHex
	extern terminalPrintString
	global boundRangeHandler
	global breakpointHandler
	global debugHandler
	global disableInterrupts
	global divisionByZeroHandler
	global doubleFaultHandler
	global enableInterrupts
	global gpFaultHandler
	global invalidOpcodeHandler
	global loadIdt
	global nmiHandler
	global noSseHandler
	global overflowHandler
	global pageFaultHandler
loadIdt:
	lidt [idtDescriptor]
	ret

disableInterrupts:
	cli
	ret

enableInterrupts:
	sti
	ret

divisionByZeroHandler:
	push rdi	; Align stack to 16-byte boundary
	mov rdi, divisionByZeroStr
	xor rsi, rsi
	mov sil, 21
	call terminalPrintString
	mov rdi, rsp
	xor rsi, rsi
	mov sil, 8
	add rdi, rsi
	call terminalPrintHex
	mov rdi, onCpuStr
	xor rsi, rsi
	mov sil, 9
	call terminalPrintString
	mov rdi, [rsp + 48]
	call terminalPrintDecimal
	mov rdi, ']'
	call terminalPrintChar
	pop rdi
	cli
	hlt
	iretq

debugHandler:
	push rdi	; Align stack to 16-byte boundary
	mov rdi, debugStr
	xor rsi, rsi
	mov sil, 20
	call terminalPrintString
	mov rdi, rsp
	xor rsi, rsi
	mov sil, 8
	add rdi, rsi
	call terminalPrintHex
	mov rdi, onCpuStr
	xor rsi, rsi
	mov sil, 9
	call terminalPrintString
	mov rdi, [rsp + 48]
	call terminalPrintDecimal
	mov rdi, ']'
	call terminalPrintChar
	pop rdi
	cli
	hlt
	iretq

nmiHandler:
	push rdi	; Align stack to 16-byte boundary
	mov rdi, nmiHandlerStr
	xor rsi, rsi
	mov sil, 12
	call terminalPrintString
	mov rdi, onCpuStr
	xor rsi, rsi
	mov sil, 9
	call terminalPrintString
	mov rdi, [rsp + 48]
	call terminalPrintDecimal
	mov rdi, ']'
	call terminalPrintChar
	pop rdi
	cli
	hlt
	iretq

breakpointHandler:
	push rdi	; Align stack to 16-byte boundary
	mov rdi, breakpointStr
	xor rsi, rsi
	mov sil, 25
	call terminalPrintString
	mov rdi, rsp
	xor rsi, rsi
	mov sil, 8
	add rdi, rsi
	call terminalPrintHex
	mov rdi, onCpuStr
	xor rsi, rsi
	mov sil, 9
	call terminalPrintString
	mov rdi, [rsp + 48]
	call terminalPrintDecimal
	mov rdi, ']'
	call terminalPrintChar
	pop rdi
	cli
	hlt
	iretq

overflowHandler:
	push rdi	; Align stack to 16-byte boundary
	mov rdi, overflowStr
	xor rsi, rsi
	mov sil, 23
	call terminalPrintString
	mov rdi, rsp
	xor rsi, rsi
	mov sil, 8
	add rdi, rsi
	call terminalPrintHex
	mov rdi, onCpuStr
	xor rsi, rsi
	mov sil, 9
	call terminalPrintString
	mov rdi, [rsp + 48]
	call terminalPrintDecimal
	mov rdi, ']'
	call terminalPrintChar
	pop rdi
	cli
	hlt
	iretq

boundRangeHandler:
	push rdi	; Align stack to 16-byte boundary
	mov rdi, boundRangeStr
	xor rsi, rsi
	mov sil, 25
	call terminalPrintString
	mov rdi, rsp
	xor rsi, rsi
	mov sil, 8
	add rdi, rsi
	call terminalPrintHex
	mov rdi, onCpuStr
	xor rsi, rsi
	mov sil, 9
	call terminalPrintString
	mov rdi, [rsp + 48]
	call terminalPrintDecimal
	mov rdi, ']'
	call terminalPrintChar
	pop rdi
	cli
	hlt
	iretq

invalidOpcodeHandler:
	push rdi	; Align stack to 16-byte boundary
	mov rdi, invalidOpcodeStr
	xor rsi, rsi
	mov sil, 19
	call terminalPrintString
	mov rdi, rsp
	xor rsi, rsi
	mov sil, 8
	add rdi, rsi
	call terminalPrintHex
	mov rdi, onCpuStr
	xor rsi, rsi
	mov sil, 9
	call terminalPrintString
	mov rdi, [rsp + 48]
	call terminalPrintDecimal
	mov rdi, ']'
	call terminalPrintChar
	pop rdi
	cli
	hlt
	iretq

noSseHandler:
	push rdi	; Align stack to 16-byte boundary
	mov rdi, noSseStr
	xor rsi, rsi
	mov sil, 28
	call terminalPrintString
	mov rdi, rsp
	xor rsi, rsi
	mov sil, 8
	add rdi, rsi
	call terminalPrintHex
	mov rdi, onCpuStr
	xor rsi, rsi
	mov sil, 9
	call terminalPrintString
	mov rdi, [rsp + 48]
	call terminalPrintDecimal
	mov rdi, ']'
	call terminalPrintChar
	pop rdi
	cli
	hlt
	iretq

doubleFaultHandler:
	mov rdi, doubleFaultStr
	xor rsi, rsi
	mov sil, 13
	call terminalPrintString
	mov rdi, onCpuStr
	xor rsi, rsi
	mov sil, 9
	call terminalPrintString
	mov rdi, [rsp + 48]
	call terminalPrintDecimal
	mov rdi, ']'
	call terminalPrintChar
	pop r8	; Pop the 64 bit error code in thrashable register
	cli
	hlt
	iretq

gpFaultHandler:
	mov rdi, gpFaultStr
	xor rsi, rsi
	mov sil, 29
	call terminalPrintString
	mov rdi, rsp
	xor rsi, rsi
	mov sil, 8
	add rdi, rsi
	call terminalPrintHex
	mov rdi, onCpuStr
	xor rsi, rsi
	mov sil, 9
	call terminalPrintString
	mov rdi, [rsp + 48]
	call terminalPrintDecimal
	mov rdi, ']'
	call terminalPrintChar
	mov rdi, withErrorStr
	xor rsi, rsi
	mov sil, 12
	call terminalPrintString
	mov rdi, rsp
	xor rsi, rsi
	mov sil, 8
	call terminalPrintHex
	pop r8	; Pop the 64 bit error code in thrashable register
	cli
	hlt
	iretq

pageFaultHandler:
	mov rdi, pageFaultStr1
	xor rsi, rsi
	mov sil, 15
	call terminalPrintString
	mov rdi, rsp
	xor rsi, rsi
	mov sil, 8
	add rdi, rsi
	call terminalPrintHex
	mov rdi, onCpuStr
	xor rsi, rsi
	mov sil, 9
	call terminalPrintString
	mov rdi, [rsp + 48]
	call terminalPrintDecimal
	mov rdi, ']'
	call terminalPrintChar
	mov rdi, pageFaultStr2
	xor rsi, rsi
	mov sil, 17
	call terminalPrintString
	push rdi	; Align stack to 16-byte boundary
	; Get the virtual address that caused this fault and display it
	mov rax, cr2
	push rax
	mov rdi, rsp
	xor rsi, rsi
	mov sil, 8
	call terminalPrintHex
	pop rax
	pop rdi
	mov rdi, withErrorStr
	xor rsi, rsi
	mov sil, 12
	call terminalPrintString
	mov rdi, rsp
	xor rsi, rsi
	mov sil, 8
	call terminalPrintHex
	pop r8	; Pop the 64 bit error code in thrashable register
	cli
	hlt
	iretq
