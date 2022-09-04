[bits 64]

section .text
	global disableLegacyPic
	global isApicPresent

disableLegacyPic:
	mov al, 0xff
	out 0xa1, al
	out 0x21, al
	ret

isApicPresent:
	mov eax, 1
	cpuid
	and edx, 1 << 9
	jnz apicPresentBlock
	mov rax, 0
	ret
apicPresentBlock:
	mov rax, 1
	ret
