[bits 64]

; Assembly level routines

section .text
	global HangSystem
	global GetPhysicalAddressLimit
	global GetLinearAddressLimit

HangSystem:
	cli
	hlt
	jmp HangSystem
	ret	; reduntant since system should not exit from here

GetPhysicalAddressLimit:
	mov eax,0x80000008
	cpuid
	and eax, 0xff
	ret

GetLinearAddressLimit:
	mov eax,0x80000008
	cpuid
	shr eax,8
	and eax,0xff
	ret