[bits 64]

section .text
	global getPhysicalAddressLimit
	global getLinearAddressLimit
	global hangSystem
	global flushTLB

flushTLB:
	cli
	mov cr3, rdi
	; FIXME: must set the interrupt flag back to its original state
	ret

hangSystem:
	cli
	hlt
	jmp hangSystem
	ret

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
