[bits 64]

KERNEL_STACK_SIZE equ 65536 ; 64 KiB stack

;Reserve space for stack
section .KERNELSTACK
kernelStack:
	times KERNEL_STACK_SIZE db 0

section .lowerhalf
	extern gdtDescriptor
	extern idtDescriptor
	extern GDT_START
	extern TSS_START
	global kernelStart

; rdi will have the address of the info table, DO NOT trash it
; Execution starts here
; 32-bit code of the loader cannot jump to 64-bit address directly
; Hence, first jump to KERNEL_LOWERHALF_ORIGIN and then jump to KERNEL_HIGHERHALF_ORIGIN
; Currently CPU is in 32-bit compatibility mode since GDT64 is not loaded
kernelStart:
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
	mov rax, higherHalfStart
	jmp rax

section .text
	global flushTLB
	global hangSystem
	global higherHalfStart
	extern kernelMain
higherHalfStart:
	mov rax, 0x00000000ffffffff
	and rdi, rax	; rdi contains info table address, pass it as 1st parameter to kernelMain
	and rsi, rax	; rsi contains kernel's lower half size, 2nd parameter
	and rdx, rax	; rdx contains kernel's higher half size, 3rd parameter
	and rcx, rax	; rcx contains usable physical memory address right after PML4 entries, 4th parameter
	call kernelMain
	; code beyond this should never get executed
kernelEnd:
	hlt
	jmp kernelEnd

flushTLB:
	cli
	mov cr3, rdi
	; FIXME: must set the interrupt flag back to its original state
	ret

hangSystem:
	mov rax, rdi
	cmp rax, 0
	jz noDisableInterrupts
	cli
noDisableInterrupts:
	hlt
	jmp hangSystem
	ret