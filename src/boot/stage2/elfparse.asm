[bits 32]
[org 0x00020000]
jmp start

BOOTLOADER_SEGMENT equ 0x07c0
L32K64_SCRATCH_BASE equ 0x80000
LOADER32_ORIGIN equ 0x00100000

times 8 - ($-$$) db 0

magicBytes db 'ELFP'
elfParseMsg db 'Parsing loader32...', 13, 10, 0

start:
	mov ecx, 19
	mov ebx, 0xb80a0
	mov esi, elfParseMsg
printStr:
	mov al, [esi]
	mov [ebx], al
	inc ebx
	mov byte [ebx], 0x72
	inc ebx
	inc esi
	loop printStr

	; 3 32-bit arguments have been passed on the stack at 0x7c00
	; These will be passed on to loader32
	mov esp, ((BOOTLOADER_SEGMENT << 4) - (3 * 4))

	mov ebx, L32K64_SCRATCH_BASE
	mov eax, [ebx + 28]	; Get address of program header array
	xor ecx, ecx
	mov cx, [ebx + 44]	; Get number of entries in the program header array
	add ebx, eax
	xor edx, edx
sectionLoadLoop:
	push ecx
	mov eax, [ebx]
	cmp eax, 1
	jne sectionLoadSkip
	mov ecx, [ebx + 20]	; Get section size in virtual memory
	; Get 4KiB aligned total virtual memory size in edx
	mov eax, ecx
	and eax, 0xfffff000
	jz sectionLoadLoop4KibSkip
	add eax, 0x1000
sectionLoadLoop4KibSkip:
	add edx, eax
	mov edi, [ebx + 8]		; Get section starting virtual address
	mov al, 0		; memset ecx bytes with 0
	rep stosb
	mov edi, [ebx + 8]
	mov esi, L32K64_SCRATCH_BASE	; Get the offset the file at which data of this section resides
	add esi, [ebx + 4]
	mov ecx, [ebx + 16]	; Get section size in the file
	rep movsb		; Copy the data
sectionLoadSkip:
	add ebx, 32		; go to next program header entry
	pop ecx
	loop sectionLoadLoop

	; Push 4th argument (virtual memory size of loader32) on stack
	push edx
	jmp 0x0008:LOADER32_ORIGIN	; loader32 is linked to start at 1MiB

end:
	cli
	hlt
	jmp end
