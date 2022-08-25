[bits 64]

section .text
	global getPhysicalAddressLimit
	global getLinearAddressLimit
	global hangSystem

hangSystem:
	cli
	hlt
	jmp hangSystem
	ret	; reduntant since system should not exit from here

getPhysicalAddressLimit:
	mov eax,0x80000008
	cpuid
	and eax, 0xff
	ret

getLinearAddressLimit:
	mov eax,0x80000008
	cpuid
	shr eax,8
	and eax,0xff
	ret