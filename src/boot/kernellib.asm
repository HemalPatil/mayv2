[bits 64]

; Assembly level routines

section .text
	global getPhysicalAddressLimit
	global getLinearAddressLimit
	global hangSystem
	global outputByte

outputByte:
	mov al, sil
	mov dx, di
	out dx, al
	ret

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