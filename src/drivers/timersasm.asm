[bits 64]

section .text
	global spinDelay
spinDelay:
	test rdi, rdi
	jz spinDelayDone
	dec rdi
	pause
	jmp spinDelay
spinDelayDone:
	ret
