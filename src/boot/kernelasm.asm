[bits 64]

KERNEL_STACK_SIZE equ 65536 ; 64 KiB stack

;Reserve space for stack
section .KERNELSTACK
kernel_stack:
	times KERNEL_STACK_SIZE db 0

section .lowerhalf
	extern GDTDescriptor
	extern IDTDescriptor
	extern __GDT_START
	extern __TSS_START
	global kernel_start

; rdi will have the address of the info table. DO NOT trash it.
; Execution starts here. 
; 32-bit code of the loader cannot jump to 64-bit address directly.
; Hence, we first jump to 0x80000000 and then jump to 0xffffffff80000000
; Currently we are in 32-bit compatibility mode since GDT64 is not loaded
kernel_start:
	mov ax, 0x10		; 64-bit data segment
	mov ds, ax
	mov es, ax
	mov ss, ax
	mov rsp, kernel_stack + KERNEL_STACK_SIZE
	xor rbp, rbp

	; Setup 64-bit GDT
	mov rax, GDTDescriptor
	lgdt [rax]	; load new GDT

	; Jump to true 64-bit long mode
	mov rax, higher_half_start
	jmp rax

section .text
	global higher_half_start
	extern KernelMain
higher_half_start:
	and rdi, 0x00000000ffffffff	; rdi contains info table address, pass it as 1st parameter to KernelMain
	call KernelMain
	; code beyond this should never get executed
kernel_end:
	hlt
	jmp kernel_end