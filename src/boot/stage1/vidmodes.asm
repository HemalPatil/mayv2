[bits 16]
[org 0x0000]
jmp start

times 8 - ($-$$) db 0

magic_bytes db 'VIDM'
vidmodes_msg db 'Getting VESA VBE 3.0 information block',13,10,0
info_block_fail db 'Cannot get VESA VBE 3.0 info. Cannot boot!',13,10,0

start:
	pusha
	push ds
	mov ax,cs
	mov ds,ax
	mov si,vidmodes_msg
	int 0x22
	
	pop ds
	popa
	retf 4
