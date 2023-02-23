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
	mov eax, 1
	cpuid
	and ecx, 1 << 30
	jz noRandom
	pop rbx
	mov rax, 1
	ret
noRandom:
	; TODO: show some error message. Cannot use terminal functions
	; since they rely on optimizations
	cli
	hlt

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
