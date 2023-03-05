; Assumes that if a processor starts executing this code,
; it supports 64-bit long mode, SSE4.2, NX bit, RDRAND,
; and all other processor capability checks done by the boot CPU

APU_BOOTLOADER_ORIGIN equ 0x8000
APU_BOOTLOADER_PADDING equ 32

[bits 16]

; Ensure execution starts at 0x1000 aligned 0x0:APU_BOOTLOADER_ORIGIN
; to flush out CS and get all linear addresses to switch directly to long mode
[org APU_BOOTLOADER_ORIGIN]
jmp 0x0:start

times APU_BOOTLOADER_PADDING - ($-$$) db 0

apuInfoTable:
	apuLongModeStart dq 0
	pml4tPhysicalAddress dq 0
gdt64Descriptor:
	dw 0
	dq 0

	; A copy of the initial GDT from kernel is
	; here because doing far jumps in 64-bit mode is tedious
	; so to change the cs selector, a copy of GDT will be loaded from here itself
	; do a far jump and then just change base of GDT to a 64-bit address
kernelGdtCopy:
	; null entry
	dq 0
	; 64 bit code segment
	dq 0x00a09a0000000000
	; 64 bit data segment
	dq 0x00a0920000000000
gdtCopyDescriptor:
	dw $ - kernelGdtCopy -1
	dd kernelGdtCopy

; Make sure this padding can fit the ApuInfoTable
times (APU_BOOTLOADER_PADDING + 128) - ($-$$) db 0

start:
	cli
	cld
	mov ax, cs
	mov ds, ax
	mov eax, [pml4tPhysicalAddress]		; Get PML4T address in eax
	mov cr3, eax			; Set CR3 to PML4T address
	mov eax, cr4
	or eax, 10100000b		; Set PGE and PAE enable bit
	mov cr4, eax
	mov ecx, 0xc0000080		; Copy contents of EFER MSR in eax
	rdmsr
	or eax, 1 << 8 | 1 << 11	; Set LM (long mode) bit and NX (execute disable) bit
	wrmsr				; Write back to EFER MSR
	mov eax, cr0
	or eax, 1 << 31 | 1			; Enable protected mode and paging
	mov cr0, eax
	lgdt [gdtCopyDescriptor]
	jmp 0x8:apuCompatibilityModeStart

[bits 64]
apuCompatibilityModeStart:
	mov ax, 0x10
	mov ds, ax
	lgdt [gdt64Descriptor]
	mov rax, [apuLongModeStart]
	jmp rax
	; code beyond this should never get executed
apuEnd:
	cli
	hlt
	jmp apuEnd
