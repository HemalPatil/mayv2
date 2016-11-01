[bits 64]

; Interrupt Tables and functions

section .IDT
	global InterruptDescriptorTable
InterruptDescriptorTable:
	dw 0