[bits 64]

KERNEL_STACK_SIZE equ 65536 ; 64 KiB stack

;Reserve space for stack
section .bss
	global kernel_stack
kernel_stack:
	resb KERNEL_STACK_SIZE

section .lowerhalf
	global kernel_start
kernel_start:		; Execution starts here. 32-bit code cannot jump to 64-bit address. Hence, we first jump to 0x80000000 and then jump to 0xffffffff80000000
	mov ax,0x38		; 64-bit data segment
	mov ds,ax
	mov ss,ax
	mov rax, higher_half_start
	jmp rax

section .text
	global higher_half_start
	global test1
	extern KernelMain
higher_half_start:
	jmp KernelMain
kernel_end:
	cli
	hlt
	jmp kernel_end

test1:
	push rbp
	mov rbp,rsp
	mov rax,[rbp+16]
	mov r14,[rbp+24]
	jmp kernel_end
	mov rsp,rbp
	pop rbp
	ret