[bits 64]

section .rodata
	APICNotPresent db 'APIC not present', 10, 0
	APICPresent db 'APIC present', 10, 0
	CheckingAPIC db 'Checking APIC presence...', 10, 0
	DisabledPIC db 10, 'Disabled PIC', 10, 0

section .text
	extern KernelPanic
	extern terminalPrintString
	global SetupAPIC

SetupAPIC:
	; Disable PIC
	mov al, 0xff
	out 0xa1, al
	out 0x21, al
	mov rdi, DisabledPIC
	mov rsi, 14
	call terminalPrintString
	; Check APIC presence
	mov rdi, CheckingAPIC
	mov rsi, 26
	call terminalPrintString
	mov eax, 1
	cpuid
	and edx, 1 << 9
	jnz APICPresentBlock
	mov rdi, APICNotPresent
	mov rsi, 17
	call terminalPrintString
	call KernelPanic
	ret	; Should never be executed
APICPresentBlock:
	mov rdi, APICPresent
	mov rsi, 13
	call terminalPrintString
	; TODO: enable APIC
	ret
