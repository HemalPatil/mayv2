[bits 64]

section .text
	global outputByte

outputByte:
	mov al, sil
	mov dx, di
	out dx, al
	ret