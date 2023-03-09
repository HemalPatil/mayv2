[bits 64]

section .text
	global disableLegacyPic
	global enableX2Apic

disableLegacyPic:
	mov al, 0xff
	out 0xa1, al
	out 0x21, al
	ret

enableX2Apic:
	xor edi, edi
	mov di, 1 << 11
	mov eax, 1
	cpuid
	xor rax, rax
	and ecx, 1 << 21
	jz x2ApicNotPresent
	and edx, 1 << 9
	jz x2ApicNotPresent
	mov ecx, 0x1b
	rdmsr
	or eax, edi		; Put local APIC in xAPIC mode
	wrmsr
	rdmsr
	shr edi, 1
	or eax, edi		; Put local APIC in x2APIC mode
	wrmsr
	xor rax, rax
	inc rax
x2ApicNotPresent:
	ret
