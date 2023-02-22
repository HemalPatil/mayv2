[bits 16]
[org 0x0000]
jmp start

times 8 - ($-$$) db 0

magicBytes db 'x64', 0
cpuidNotAvailable db 'CPUID not available.', 13, 10, 0
noX64 db 'Processor does not support either 64-bit mode or NX bit.', 13, 10, 0
cannotBootMsg db 'Cannot boot!', 13, 10, 0
longMode db '64-bit long mode and NX bit available', 13, 10, 13, 10, 0

start:
	pusha
	push ds
	mov ax, cs
	mov ds, ax
	
checkCpuid:
	pushfd
	pop eax
	mov ecx, eax
	xor eax, 1 << 21
	push eax
	popfd
	pushfd
	pop eax
	push ecx
	popfd
	xor eax, ecx
	jz noCpuid

checkExtensions:
	mov eax, 0x80000000
	cpuid
	cmp eax, 0x80000001
	jb noLongMode

checkX64AndNXBit:
	mov eax, 0x80000001
	cpuid
	test edx, 1 << 29
	jz noLongMode
	test edx, 1 << 20
	jz noLongMode

	mov si, longMode
	int 0x22
	jmp end

noCpuid:
	mov si, cpuidNotAvailable
	int 0x22
	jmp error

noLongMode:
	mov si, noX64
	int 0x22

error:
	mov si, cannotBootMsg
	int 0x22
errorEnd:
	cli
	hlt
	jmp errorEnd
	
end:
	pop ds
	popa
	retf
