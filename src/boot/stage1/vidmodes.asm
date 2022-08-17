[bits 16]
[org 0x0000]
jmp start

times 8 - ($-$$) db 0

magicBytes db 'VIDM'
vidModesMsg db 'Getting VESA VBE 3.0 information block', 13, 10, 0
infoBlockFail db 'Cannot get VESA VBE 3.0 info. Cannot boot!', 13, 10, 0

start:
	pusha
	push ds
	mov ax, cs
	mov ds, ax
	mov si, vidModesMsg
	int 0x22
	
	pop ds
	popa
	retf 4
