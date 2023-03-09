[bits 64]

CPU_STACK_SIZE equ 0x10000

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

section .text
	global flushTLB
	global haltSystem
	global hangSystem
	global loadTss
	global perpetualWait
	global prepareApuInfoTable
	global readMsr
	global writeMsr
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

loadTss:
	ltr di
	ret

perpetualWait:
	hlt
	jmp perpetualWait

prepareApuInfoTable:
	mov qword [rdi], apuLongModeStart
	mov [rdi + 8], rsi
	sgdt [rdi + 16]
	ret

readMsr:
	mov ecx, edi
	xor rax, rax
	rdmsr
	shl rdx, 32
	xor edx, edx
	or rax, rdx
	ret

writeMsr:
	mov ecx, edi
	mov rax, rsi
	mov rdx, rsi
	shr rdx, 32
	wrmsr
	ret
