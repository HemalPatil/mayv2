[bits 64]

section .text
	global disableLegacyPic
	global isX2ApicPresent

disableLegacyPic:
	mov al, 0xff
	out 0xa1, al
	out 0x21, al
	ret

isX2ApicPresent:
	mov eax, 1
	cpuid
	xor rax, rax
	and ecx, 1 << 21
	jz x2ApicNotPresent
	and edx, 1 << 9
	jz x2ApicNotPresent
	inc rax
x2ApicNotPresent:
	ret
