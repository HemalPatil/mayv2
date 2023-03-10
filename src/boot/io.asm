[bits 64]

section .text
	global inputByte
	global outputByte

inputByte:
	mov rdx, rdi
	xor rax, rax
	in al, dx
	ret

outputByte:
	mov al, sil
	mov dx, di
	out dx, al
	ret
