[bits 64]

KERNEL_STACK_SIZE equ 65536 ; 64KiB stack

;Reserve space for stack
section .bss
kernelStack:
	resb KERNEL_STACK_SIZE

section .bpuorigin progbits alloc exec nowrite align=0x1000
	extern gdtDescriptor
	global bpuCompatibilityModeStart

; Kernel64 execution starts here
; rdi, rsi, rdx, and rcx will have important kernel parameters, DO NOT trash them
; 32-bit code of the loader cannot jump to 64-bit address directly
; Hence, first jump to bpuCompatibilityModeStart (BPU_COMPAT_MODE_ORIGIN) and then jump to bpuLongModeStart
; Currently CPU is in 32-bit compatibility mode since GDT64 is not loaded
bpuCompatibilityModeStart:
	mov ax, 0x10		; 64-bit data segment
	mov ds, ax
	mov es, ax
	mov ss, ax
	mov rsp, kernelStack + KERNEL_STACK_SIZE
	xor rbp, rbp

	; Setup 64-bit GDT
	lgdt [gdtDescriptor]

	; Jump to true 64-bit long mode
	mov rax, bpuLongModeStart
	jmp rax

section .text
	extern bpuMain
	global flushTLB
	global haltSystem
	global hangSystem
	global prepareApuInfoTable
bpuLongModeStart:
	mov rax, 0x00000000ffffffff
	and rdi, rax	; rdi contains info table address, pass it as 1st parameter to bpuMain
	and rsi, rax	; rsi contains boot CPU compatibility mode segment size
	and rdx, rax	; rdx contains kernel's higher half size, 3rd parameter
	and rcx, rax	; rcx contains usable physical memory address right after PML4 entries, 4th parameter
	call bpuMain
	; code beyond this should never get executed
bpuEnd:
	cli
	hlt
	jmp bpuEnd

apuLongModeStart:
	mov r8, 0x0807060504030201
	cli
	hlt

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

prepareApuInfoTable:
	mov qword [rdi], apuLongModeStart
	mov [rdi + 8], rsi
	sgdt [rdi + 16]
	ret
