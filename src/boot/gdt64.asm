[bits 64]

; GDT for 64 bit mode

; Access bits
PRESENT        equ 1 << 7
NOT_SYS        equ 1 << 4
EXEC           equ 1 << 3
DC             equ 1 << 2
RW             equ 1 << 1
ACCESSED       equ 1 << 0
 
; Flags bits
GRAN_4K       equ 1 << 7	; 4KiB granularity
SZ_32         equ 1 << 6
LONG_MODE     equ 1 << 5

section .GDT64
	global __GDT_START
	global __GDT_END
	global GDTDescriptor

__GDT_START:
; null entry
	dq 0

; selector 0x8 - 64 bit code segment descriptor
	dw 0	; limit[0..15], ignored
	dw 0	; base[0..15], ignored
	db 0	; base[16..23], ignored
	db PRESENT | NOT_SYS | EXEC | RW	; Access
	db GRAN_4K | LONG_MODE	; limit[16..19] (low), ignored
	db 0	; base[24..31], ignored

; selector 0x10 - 64 bit data segment descriptor
	dw 0	; limit[0..15], ignored
	dw 0	; base[0..15], ignored
	db 0	; base[16..23], ignored
	db PRESENT | NOT_SYS | RW	; Access
	db GRAN_4K | LONG_MODE	; limit[16..19] (low), ignored
	db 0	; base[24..31], ignored

; selector 0x18 - 64 bit TSS descriptor
	; TSS base address to be filled in at runtime
	dw 104	; TSS size
	dw 0	; TSS base[0..15]
	db 0	; TSS base[16..23]
	db 0x89	; TSS access flags
	db 0x90	; TSS flags and limit[16..19]
	db 0	; base[24..31]
	dq 0	; TSS base[32..63] and reserved dword

	times 4096 - ($-$$) db 0	; Make the GDT 4 KiB long

__GDT_END:

section .rodata:
GDTDescriptor:
	GDT64Limit dw 65535
	GDT64Base dq __GDT_START