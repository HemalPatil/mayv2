[bits 64]

CPU_STACK_SIZE equ 65536

;Reserve space for BPU stack
section .bss align=16
bpuStack:
	resb CPU_STACK_SIZE

; Kernel64 execution starts here
; rdi, rsi, and rdx will have important kernel parameters, DO NOT trash them
section .text.bpuorigin
	extern bpuMain
	extern gdtDescriptor
bpuLongModeStart:
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
	mov rsp, bpuStack + CPU_STACK_SIZE
	xor rbp, rbp

	; Setup 64-bit GDT
	lgdt [gdtDescriptor]

	call bpuMain
	; code beyond this should never get executed
bpuEnd:
	cli
	hlt
	jmp bpuEnd

section .text
	global flushTLB
	global haltSystem
	global hangSystem
	global prepareApuInfoTable
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
