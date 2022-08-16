[bits 64]

section .rodata
	apicNotPresent db 'APIC not present', 10, 0
	apicPresent db 'APIC present', 10, 0
	checkingApic db 'Checking APIC presence...', 10, 0
	disabledPic db 10, 'Disabled PIC', 10, 0

section .text
	extern kernelPanic
	extern terminalPrintString
	global setupApic

setupApic:
	; Disable PIC
	mov al, 0xff
	out 0xa1, al
	out 0x21, al
	mov rdi, disabledPic
	mov rsi, 14
	call terminalPrintString
	; Check APIC presence
	mov rdi, checkingApic
	mov rsi, 26
	call terminalPrintString
	mov eax, 1
	cpuid
	and edx, 1 << 9
	jnz apicPresentBlock
	mov rdi, apicNotPresent
	mov rsi, 17
	call terminalPrintString
	call kernelPanic
	ret	; Should never be executed
apicPresentBlock:
	mov rdi, apicPresent
	mov rsi, 13
	call terminalPrintString
	; TODO: enable APIC
	ret
