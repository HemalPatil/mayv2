[bits 64]

; GDT for 64 bit mode

; Access bits
PRESENT        equ 1 << 7
NOT_SYS        equ 1 << 4
EXEC           equ 1 << 3
RW             equ 1 << 1
ACCESSED       equ 1 << 0
 
; Flags bits
GRAN_4K       equ 1 << 7	; 4KiB granularity
SZ_32         equ 1 << 6
LONG_MODE     equ 1 << 5

section .data
GDT_START:
; null entry
	dq 0

; selector 0x8 - 64-bit code segment descriptor
	dw 0	; limit[0..15], ignored
	dw 0	; base[0..15], ignored
	db 0	; base[16..23], ignored
	db PRESENT | NOT_SYS | EXEC | RW	; Access
	db GRAN_4K | LONG_MODE	; limit[16..19] (low), ignored
	db 0	; base[24..31], ignored

; selector 0x10 - 64-bit data segment descriptor
	dw 0	; limit[0..15], ignored
	dw 0	; base[0..15], ignored
	db 0	; base[16..23], ignored
	db PRESENT | NOT_SYS | RW	; Access
	db GRAN_4K | LONG_MODE	; limit[16..19] (low), ignored
	db 0	; base[24..31], ignored

	times 4096 - ($-$$) db 0	; Make the GDT 4 KiB long
GDT_END:

section .rodata align=16
	global gdt64Base
	global gdtDescriptor
gdtDescriptor:
	gdt64Limit dw GDT_END - GDT_START - 1
	gdt64Base dq GDT_START
