[bits 64]

KERNEL_STACK_SIZE equ 65536 ; 64 KiB stack

;Reserve space for stack
section .bss
	global kernel_stack
kernel_stack:
	resb KERNEL_STACK_SIZE

section .text
	global kernel_start
kernel_start:
kernel_end:
	cli
	hlt
	jmp kernel_end
