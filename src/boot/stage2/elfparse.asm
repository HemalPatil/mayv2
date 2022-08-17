[bits 32]
[org 0x20000]
jmp start

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

	mov esp, 0x7bf4		; 4 32-bit arguments have been passed on the stack at 0x7c00
	
	mov ebx, 0x80000
	mov eax, [ebx + 28]	; Get address of program header array
	xor ecx, ecx
	mov cx, [ebx + 44]		; Get number of entries in the program header array
	add ebx, eax
sectionLoadLoop:
	push ecx
	mov eax, [ebx]
	cmp eax, 1
	jne sectionLoadSkip
	mov ecx, [ebx + 20]	; Get section size in virtual memory
	mov edi, [ebx + 8]		; Get section starting virtual address
	mov al, 0		; memset ecx bytes with 0
	rep stosb
	mov edi, [ebx + 8]
	mov esi, 0x80000		; Get the offset the file at which data of this
	add esi, [ebx + 4]		; section resides
	mov ecx, [ebx + 16]	; Get section size in the file
	rep movsb		; Copy the data
sectionLoadSkip:
	add ebx, 32		; go to next program header entry
	pop ecx
	loop sectionLoadLoop
	
	jmp 0x0008:0x00100000	; Our kernel is linked to start at 0x100000 i.e. 1MiB
	
end:
	cli
	hlt
	jmp end
