[bits 16]
[org 0x0000]

BOOTLOADER_SEGMENT equ 0x07c0

jmp BOOTLOADER_SEGMENT:start	; Ensure cs=0x07c0

start:
	cli
	mov ebx, 0xcafebabe
	mov edx, 0x01050206
	hlt
