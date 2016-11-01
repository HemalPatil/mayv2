[bits 64]

; Assembly level routines

section .text
	global HaltSystem
HaltSystem:
	cli
	hlt
	jmp HaltSystem
	ret	; reduntant since system should exit from here