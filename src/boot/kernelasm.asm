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

; rdi will have the address of the info table. DO NOT trash it.
; Execution starts here. 
; 32-bit code of the loader cannot jump to 64-bit address directly.
; Hence, we first jump to 0x80000000 and then jump to 0xffffffff80000000
; Currently we are in 32-bit compatibility mode since GDT64 is not loaded
kernelStart:
	mov ax, 0x10		; 64-bit data segment
	mov ds, ax
	mov es, ax
	mov ss, ax
	mov rsp, kernelStack + KERNEL_STACK_SIZE
	xor rbp, rbp

	; Setup 64-bit GDT
	mov rax, gdtDescriptor
	lgdt [rax]	; load new GDT

	; Jump to true 64-bit long mode
	mov rax, higherHalfStart
	jmp rax

section .text
	global higherHalfStart
	extern kernelMain
higherHalfStart:
	mov rax, 0x00000000ffffffff
	and rdi, rax	; rdi contains info table address, pass it as 1st parameter to kernelMain
	and rsi, rax	; rsi contains kernel ELF base, 2nd parameter
	and rdx, rax	; rdx contains usable memory address right after PML4 entries
	call kernelMain
	; code beyond this should never get executed
kernelEnd:
	hlt
	jmp kernelEnd