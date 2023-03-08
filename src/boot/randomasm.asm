[bits 64]

section .rodata
	randomFailedStr db 'Random::getRandom64 failed', 10, 0

section .text
	extern panic
	extern terminalPrintString
	global getRandom64
	global isRandomAvailable
isRandomAvailable:
	push rbx	; Preserve rbx to stay compatible with System V ABI
	xor eax, eax
	inc eax
	cpuid
	pop rbx
	xor rax, rax
	and ecx, 1 << 30
	jz noRandom
	inc rax
noRandom:
	ret

getRandom64:
	mov ecx, 100
getRandom64Retry:
	rdrand rax
	jc getRandom64Success
	loop getRandom64Retry
	mov rdi, randomFailedStr
	mov rsi, 27
	call terminalPrintString
	call panic
getRandom64Success:
	ret
