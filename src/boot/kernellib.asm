[bits 64]

; Assembly level routines

section .text
	global HangSystem
HangSystem:
	cli
	hlt
	jmp HangSystem
	ret	; reduntant since system should not exit from here