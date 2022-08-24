[bits 32]

LOADER_STACK_SIZE equ 4096

section .bss
	global loaderStack
loaderStack:
	resb LOADER_STACK_SIZE

section .setup
	global loader32Start
	extern loader32Main
loader32Start:
	cli
	mov ebp, 0x7bf0		; Point ebp to parameters that were passed on the stack at 0x7c00
	mov esp, loaderStack + LOADER_STACK_SIZE		; Setup LOADER32 stack
	mov eax, [ebp + 12]		; 4 32-bit parameters were passed
	push eax
	mov eax, [ebp + 8]
	push eax
	mov eax, [ebp + 4]
	push eax
	mov eax, [ebp]
	push eax
	call loader32Main
end:
	cli			; Halt the processor
	hlt			; Hang the proceesor by disabling interrupts
	jmp end			; loader32 will never return to this code
