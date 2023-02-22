[bits 64]

KERNEL_STACK_SIZE equ 65536 ; 64 KiB stack

;Reserve space for stack
section .bss
kernelStack:
	resb KERNEL_STACK_SIZE

section .lowerhalf progbits alloc exec nowrite align=4096
	extern gdtDescriptor
	global kernelCompatibilityModeStart

; Kernel64 execution starts here
; rdi, rsi, rdx, and rcx will have important kernel parameters, DO NOT trash them
; 32-bit code of the loader cannot jump to 64-bit address directly
; Hence, first jump to KERNEL_LOWERHALF_ORIGIN and then jump to KERNEL_HIGHERHALF_ORIGIN
; Currently CPU is in 32-bit compatibility mode since GDT64 is not loaded
kernelCompatibilityModeStart:
	mov ax, 0x10		; 64-bit data segment
	mov ds, ax
	mov es, ax
	mov ss, ax
	mov rsp, kernelStack + KERNEL_STACK_SIZE
	xor rbp, rbp

	; Setup 64-bit GDT
	mov rax, gdtDescriptor
	lgdt [rax]

	; Jump to true 64-bit long mode
	mov rax, kernelLongModeStart
	jmp rax

section .text
	global flushTLB
	global haltSystem
	global hangSystem
	global kernelLongModeStart
	extern kernelMain
kernelLongModeStart:
	mov rax, 0x00000000ffffffff
	and rdi, rax	; rdi contains info table address, pass it as 1st parameter to kernelMain
	and rsi, rax	; rsi contains kernel's lower half size, 2nd parameter
	and rdx, rax	; rdx contains kernel's higher half size, 3rd parameter
	and rcx, rax	; rcx contains usable physical memory address right after PML4 entries, 4th parameter
	call kernelMain
	; code beyond this should never get executed
kernelEnd:
	cli
	hlt
	jmp kernelEnd

flushTLB:
	cli
	mov cr3, rdi
	; FIXME: must set the interrupt flag back to its original state
	ret

haltSystem:
	hlt
	ret

hangSystem:
	cli
	hlt
	jmp hangSystem
