[bits 64]

section .rodata:
	loadSse2Str db 'Loading SSE2', 0

section .data
align 16
floatSaveRegion:
	times 512 db 0

section .text
	extern doneStr
	extern ellipsisStr
	extern failedStr
	extern terminalPrintChar
	extern terminalPrintString
	global setupSse2
setupSse2:
	mov rdi, loadSse2Str
	mov rsi, 12
	call terminalPrintString
	mov rdi, [ellipsisStr]
	mov rsi, 3
	call terminalPrintString
	mov eax, 1
	cpuid
	mov eax, edx
	shr eax, 19
	and eax, 1
	jz noSse2
	mov eax, edx
	shr eax, 24
	and eax, 1
	jz noSse2
	mov  eax, edx
	shr eax, 25
	and eax, 1
	jz noSse2
	mov  eax, edx
	shr eax, 26
	and eax, 1
	jz noSse2
	mov rax, cr0
	and ax, 0xfffb		; clear coprocessor emulation CR0.EM
	or ax, 0x2			; set coprocessor monitoring CR0.MP
	mov cr0, rax
	mov rax, cr4
	or ax, 3 << 9		; set CR4.OSFXSR and CR4.OSXMMEXCPT at the same time
	mov cr4, rax
	mov rdi, [doneStr]
	mov rsi, 4
	call terminalPrintString
	mov rdi, 10
	call terminalPrintChar
	mov rax, 1
	ret
noSse2:
	mov rdi, [failedStr]
	mov rsi, 6
	call terminalPrintString
	mov rdi, 10
	call terminalPrintChar
	mov rax, 0
	ret
