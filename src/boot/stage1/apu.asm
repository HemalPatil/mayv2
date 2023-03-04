[bits 16]
[org 0x0000]

APU_BOOTLOADER_SEGMENT equ 0x0800

jmp APU_BOOTLOADER_SEGMENT:start	; Ensure execution starts at 0x1000 aligned 0x800:0x0

start:
	cli
	mov ebx, 0xcafebabe
	mov edx, 0x01050206
	hlt
