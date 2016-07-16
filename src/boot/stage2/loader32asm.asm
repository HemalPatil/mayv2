[bits 32]

KERNEL_STACK_SIZE equ 4096

section .bss
	global KernelStack
KernelStack:
	resb KERNEL_STACK_SIZE

section .text
	global kernel32start		; Kernel execution starts here
	extern Kernel32Main
kernel32start:
	cli
	mov ebp,0x7bf4		; Point ebp to parameters that were passed on the stack at 0x7c00
	mov esp,KernelStack + KERNEL_STACK_SIZE		; setup KERNEL32 stack
	mov eax,[ebp+8]		; 3 parameters were passed
	push eax
	mov eax,[ebp+4]
	push eax
	mov eax,[ebp]
	push eax
	mov ebx,0xcafebabe		; Random test value in ebx
	call Kernel32Main		; Call the actual kernel32 (written in C)
end:
	cli			; Halt the processor
	hlt			; Hang the proceesor by disabling interrupts
	jmp end			; (Actual kernel will never return to this code)
