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
	pageFaultStr1 db 10, 'Page Fault! ', 0
	pageFaultStr2 db ' tried to access ', 0
	pageFaultStr3 db ' with error ', 0
	invalidOpcodeStr db 'Invalid opcode', 0
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
	;==========================
	mov rdx, IDT_START
	mov rax, epHandler0
	call fillOffsets
	add rdx, 16
	mov rax, epHandler1
	call fillOffsets
	add rdx, 16
	mov rax, epHandler2
	call fillOffsets
	add rdx, 16
	mov rax, epHandler3
	call fillOffsets
	add rdx, 16
	mov rax, epHandler4
	call fillOffsets
	add rdx, 16
	mov rax, epHandler5
	call fillOffsets
	add rdx, 16
	mov rax, epHandler6
	call fillOffsets
	add rdx, 16
	mov rax, epHandler7
	call fillOffsets
	add rdx, 16
	mov rax, epHandler8
	call fillOffsets
	add rdx, 16
	mov rax, epHandler9
	call fillOffsets
	add rdx, 16
	mov rax, epHandler10
	call fillOffsets
	add rdx, 16
	mov rax, epHandler11
	call fillOffsets
	add rdx, 16
	mov rax, epHandler12
	call fillOffsets
	add rdx, 16
	mov rax, epHandler13
	call fillOffsets
	add rdx, 16
	mov rax, epHandler14
	call fillOffsets
	add rdx, 16
	mov rax, epHandler15
	call fillOffsets
	add rdx, 16
	mov rax, epHandler16
	call fillOffsets
	add rdx, 16
	mov rax, epHandler17
	call fillOffsets
	add rdx, 16
	mov rax, epHandler18
	call fillOffsets
	add rdx, 16
	mov rax, epHandler19
	call fillOffsets
	add rdx, 16
	mov rax, epHandler20
	call fillOffsets
	add rdx, 16
	mov rax, epHandler21
	call fillOffsets
	add rdx, 16
	mov rax, epHandler22
	call fillOffsets
	add rdx, 16
	mov rax, epHandler23
	call fillOffsets
	add rdx, 16
	mov rax, epHandler24
	call fillOffsets
	add rdx, 16
	mov rax, epHandler25
	call fillOffsets
	add rdx, 16
	mov rax, epHandler26
	call fillOffsets
	add rdx, 16
	mov rax, epHandler27
	call fillOffsets
	add rdx, 16
	mov rax, epHandler28
	call fillOffsets
	add rdx, 16
	mov rax, epHandler29
	call fillOffsets
	add rdx, 16
	mov rax, epHandler30
	call fillOffsets
	add rdx, 16
	mov rax, epHandler31
	call fillOffsets
	add rdx, 16
	;==========================
	; mov rdx, IDT_START + 13 * 16
	; mov qword [rdx], 0
	; add rdx, 8
	; mov qword [rdx], 0
	; mov rdx, IDT_START + 8 * 16
	; mov qword [rdx], 0
	; add rdx, 8
	; mov qword [rdx], 0
	;==========================
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
	call hangSystem
	ret

disableInterrupts:
	cli
	ret

enableInterrupts:
	sti
	ret

doubleFaultHandler:
	mov rdi, doubleFaultStr
	mov rsi, 14
	call terminalPrintString
	pop r8	; Pop the 64 bit error code in thrashable register
	iretq

pageFaultHandler:
	mov rdi, pageFaultStr1
	mov rsi, 13
	call terminalPrintString
	; Display the instruction and virtual address that caused this fault along with error
	mov rdi, rsp
	add rdi, 8
	mov rsi, 8
	call terminalPrintHex
	mov rdi, pageFaultStr2
	mov rsi, 17
	call terminalPrintString
	mov rax, cr2
	push rax
	mov rdi, rsp
	mov rsi, 8
	call terminalPrintHex
	pop rax
	mov rdi, pageFaultStr3
	mov rsi, 12
	call terminalPrintString
	mov rdi, rsp
	mov rsi, 8
	call terminalPrintHex
	mov rdi, 10
	call terminalPrintChar
	; Pop the 8-byte aligned 32-bit error code in thrashable register
	pop r8
	iretq

defaultInterruptHandler:
	mov rdi, defaultInterruptStr
	mov rsi, 27
	call terminalPrintString
	cli
	hlt
	iretq

epHandler0:
	mov rax, 0
	mov rdx, 0xdeadbeefdeadbeef
	cli
	hlt
epHandler1:
	mov rax, 1
	mov rdx, 0xdeadbeefdeadbeef
	cli
	hlt
epHandler2:
	mov rax, 2
	mov rdx, 0xdeadbeefdeadbeef
	cli
	hlt
epHandler3:
	mov rax, 3
	mov rdx, 0xdeadbeefdeadbeef
	cli
	hlt
epHandler4:
	mov rax, 4
	mov rdx, 0xdeadbeefdeadbeef
	cli
	hlt
epHandler5:
	mov rax, 5
	mov rdx, 0xdeadbeefdeadbeef
	cli
	hlt
epHandler6:
	mov rax, 6
	mov rdx, 0xdeadbeefdeadbeef
	cli
	hlt
epHandler7:
	mov rax, 7
	mov rdx, 0xdeadbeefdeadbeef
	cli
	hlt
epHandler8:
	mov rax, 8
	mov rdx, 0xdeadbeefdeadbeef
	cli
	hlt
epHandler9:
	mov rax, 9
	mov rdx, 0xdeadbeefdeadbeef
	cli
	hlt
epHandler10:
	mov rax, 10
	mov rdx, 0xdeadbeefdeadbeef
	cli
	hlt
epHandler11:
	mov rax, 11
	mov rdx, 0xdeadbeefdeadbeef
	cli
	hlt
epHandler12:
	mov rax, 12
	mov rdx, 0xdeadbeefdeadbeef
	cli
	hlt
epHandler13:
	mov rax, 13
	mov rdx, 0xdeadbeefdeadbeef
	mov rcx, [rsp + 16]
	cli
	hlt
epHandler14:
	mov rax, 14
	mov rdx, 0xdeadbeefdeadbeef
	cli
	hlt
epHandler15:
	mov rax, 15
	mov rdx, 0xdeadbeefdeadbeef
	cli
	hlt
epHandler16:
	mov rax, 16
	mov rdx, 0xdeadbeefdeadbeef
	cli
	hlt
epHandler17:
	mov rax, 17
	mov rdx, 0xdeadbeefdeadbeef
	cli
	hlt
epHandler18:
	mov rax, 18
	mov rdx, 0xdeadbeefdeadbeef
	cli
	hlt
epHandler19:
	mov rax, 19
	mov rdx, 0xdeadbeefdeadbeef
	cli
	hlt
epHandler20:
	mov rax, 20
	mov rdx, 0xdeadbeefdeadbeef
	cli
	hlt
epHandler21:
	mov rax, 21
	mov rdx, 0xdeadbeefdeadbeef
	cli
	hlt
epHandler22:
	mov rax, 22
	mov rdx, 0xdeadbeefdeadbeef
	cli
	hlt
epHandler23:
	mov rax, 23
	mov rdx, 0xdeadbeefdeadbeef
	cli
	hlt
epHandler24:
	mov rax, 24
	mov rdx, 0xdeadbeefdeadbeef
	cli
	hlt
epHandler25:
	mov rax, 25
	mov rdx, 0xdeadbeefdeadbeef
	cli
	hlt
epHandler26:
	mov rax, 26
	mov rdx, 0xdeadbeefdeadbeef
	cli
	hlt
epHandler27:
	mov rax, 27
	mov rdx, 0xdeadbeefdeadbeef
	cli
	hlt
epHandler28:
	mov rax, 28
	mov rdx, 0xdeadbeefdeadbeef
	cli
	hlt
epHandler29:
	mov rax, 29
	mov rdx, 0xdeadbeefdeadbeef
	cli
	hlt
epHandler30:
	mov rax, 30
	mov rdx, 0xdeadbeefdeadbeef
	cli
	hlt
epHandler31:
	mov rax, 31
	mov rdx, 0xdeadbeefdeadbeef
	cli
	hlt

invalidOpcodeHandler:
	mov rdi, invalidOpcodeStr
	mov rsi, 14
	call terminalPrintString
	; TODO: better recovery from invalid opcode
	cli
	hlt
	pop r8	; Pop the 64 bit error code in thrashable register
	iretq
