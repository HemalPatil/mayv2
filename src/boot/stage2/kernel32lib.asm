[bits 32]

section .data
	global hexspace
hexspace db '0123456789ABCDEF',0

section .text
	global memset
	global PrintHex
	global swap
	global GetPhysicalAddressLimit
	global GetLinearAddressLimit
	extern TerminalPutChar

PrintHex:
	push ebp
	mov ebp,esp
	pushad
	mov ecx,[ebp+12]
	mov esi,[ebp+8]
	add esi,ecx
	dec esi
PrintHexLoop:
	push ecx
	push esi
	mov dl,[esi]
	call PrintHexCore
	pop esi
	dec esi
	pop ecx
	loop PrintHexLoop
	popad
	mov esp,ebp
	pop ebp
	ret

PrintHexCore:
	pushad
	push edx
	mov ebx,hexspace
	xor eax,eax
	mov al,dl
	shr al,4
	xlat
	push 0x0f
	push eax
	call TerminalPutChar
	add esp,8
	pop edx
	mov ebx,hexspace
	xor eax,eax
	mov al,dl
	and al,0x0f
	xlat
	push 0x0f
	push eax
	call TerminalPutChar
	add esp,8
	popad
	ret
	
swap:
	push ebp
	mov ebp,esp
	pushad
	mov esi,[ebp+8]
	mov edi,[ebp+12]
	mov eax,[ebp+16]
	test eax,eax
	jz swapEnd
	xor edx,edx
	mov ecx,4
	div ecx
	push edx
	test eax,eax
	jz swapDWORDskip
	mov ecx,eax
swapDWORDLoop:
	mov eax,[esi]
	xchg eax,[edi]
	mov [esi],eax
	add esi,4
	add edi,4
	loop swapDWORDLoop
swapDWORDskip:
	pop ecx
	test ecx,ecx
	jz swapEnd
swapBYTELoop:
	mov al,[esi]
	xchg al,[edi]
	mov [esi],al
	inc esi
	inc edi
	loop  swapBYTELoop
swapEnd:	
	popad
	mov esp,ebp
	pop ebp
	ret

GetPhysicalAddressLimit:
	mov eax,0x80000008
	cpuid
	ret

GetLinearAddressLimit:
	mov eax,0x80000008
	cpuid
	shr eax,8
	ret

memset:
	push ebp
	mov ebp,esp
	pushad
	mov edi,[ebp + 8]
	mov eax,[ebp + 16]
	test eax,eax
	jz memsetEnd
	xor edx,edx
	mov ecx,4
	div ecx
	push edx
	test eax,eax
	jz memsetDWORDskip
	push eax
	mov ecx,4
memsetEAXloop:
	shl eax,8
	mov al,[ebp + 12]
	loop memsetEAXloop
	pop ecx
	rep stosd
memsetDWORDskip:
	pop ecx
	test ecx,ecx
	jz memsetEnd
	mov al,[ebp + 12]
	rep stosb
memsetEnd:
	popad
	mov esp,ebp
	pop ebp
	ret
