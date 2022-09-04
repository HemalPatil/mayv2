[bits 64]

section .text
	global hangSystem
	global flushTLB

flushTLB:
	cli
	mov cr3, rdi
	; FIXME: must set the interrupt flag back to its original state
	ret

hangSystem:
	cli
	hlt
	jmp hangSystem
	ret
