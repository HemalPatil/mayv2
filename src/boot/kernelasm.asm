[bits 64]

KERNEL_STACK_SIZE equ 65536 ; 64 KiB stack

;Reserve space for stack
section .bss
	global kernel_stack
kernel_stack:
	resb KERNEL_STACK_SIZE

section .lowerhalf
	extern GDTDescriptor
	extern __GDT_START
	extern __TSS_START
	global kernel_start

	; rdi will have the address of the info table. DO NOT trash it.
kernel_start:		; Execution starts here. 32-bit code of the loader cannot jump to 64-bit address directly. Hence, we first jump to 0x80000000 and then jump to 0xffffffff80000000
	mov ax,0x10		; 64-bit data segment
	mov ds,ax
	mov es,ax
	mov ss,ax

	; Setup 64-bit GDT and TSS
	mov rax, GDTDescriptor
	lgdt [rax]	; load new GDT

	; Setup TSS
	mov rdx, __GDT_START
	add rdx, 0x1a	; point to TSS descriptor byte 2
	mov rax, __TSS_START
	mov [rdx], ax	; move base[0..16] of TSS
	shr rax, 16
	mov [rdx + 2], al	; move base[16..23] of TSS
	shr rax, 8
	mov [rdx + 5], al	; move base[24..31] of TSS 
	shr rax, 8
	mov [rdx + 6], eax	; move base[32..63] of TSS

setup_tss:
	mov ax, 0x18
	ltr ax

	mov rax, higher_half_start	; Since higher half of the kernel is at a distance of more than 2 GiB, we put the address of higher half in RAX and then jump to it
	jmp rax

section .text
	global higher_half_start
	extern KernelMain
higher_half_start:
	and rdi, 0x00000000ffffffff	; rdi contains info table address, pass it as 1st parameter to KernelMain
	call KernelMain
kernel_end:
	cli
	hlt
	jmp kernel_end