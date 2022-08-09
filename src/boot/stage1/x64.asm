[bits 16]
[org 0x0000]
jmp start

times 8 - ($-$$) db 0

magic_bytes db 'x64', 0
cpuid_not_available db 'CPUID not available.', 13, 10, 0
no_x64 db 'Processor does not support 64-bit mode.', 13, 10, 0
cannot_boot db 'Cannot boot!', 13, 10, 0
longmode db '64-bit long mode available', 13, 10, 13, 10, 0

start:
	pusha
	push ds
	mov ax, cs
	mov ds, ax
	
check_cpuid:
	pushfd
	pop eax
	mov ecx, eax
	xor eax, 1<<21
	push eax
	popfd
	pushfd
	pop eax
	push ecx
	popfd
	xor eax, ecx
	jz no_cpuid

check_extensions:
	mov eax, 0x80000000
	cpuid
	cmp eax, 0x80000001
	jb no_longmode

check_x64:
	mov eax, 0x80000001
	cpuid
	test edx, 1<<29
	jz no_longmode

	mov si, longmode
	int 22h
	jmp end

no_cpuid:
	mov si, cpuid_not_available
	int 22h
	jmp error

no_longmode:
	mov si, no_x64
	int 22h

error:
	mov si, cannot_boot
	int 22h
error_end:
	cli
	hlt
	jmp error_end
	
end:
	pop ds
	popa
	retf
