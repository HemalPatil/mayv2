[bits 32]

section .rodata
	global hexspace
hexspace db '0123456789ABCDEF',0

section .data
k64load_offset dw 0
k64load_segment dw 0
dap_kernel_offset dw 0
dap_kernel_segment dw 0
disk_number dw 0

section .text
	global memset
	global memcopy
	global PrintHex
	global swap
	global GetPhysicalAddressLimit
	global GetLinearAddressLimit
	global Setup16BitSegments
	global LoadKernelELFSectors
	global JumpToKernel
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
	and eax,0xff
	cpuid
	ret

GetLinearAddressLimit:
	mov eax,0x80000008
	cpuid
	shr eax,8
	and eax,0xff
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

memcopy:
	push ebp	; Create stack frame
	mov ebp,esp
	pushad
	mov ax,ds	; Copy ds to es
	mov es,ax
	mov esi,[ebp+8]		; Get source pointer in ESI
	mov edi,[ebp+12]	; Get destination pointer in EDI
	mov eax,[ebp+16]	; Get count in EAX
	test eax,eax		; If count is 0, exit
	jz memcopyEnd
	xor edx,edx		; Divide count by 4
	mov ecx,4
	div ecx
	push edx	; Save the remainder
	test eax,eax	; Check if quotient is 0, if 0 then skip DWORD loop
	jz memcopyDWORDskip
	mov ecx,eax		; copy quotient to ECX
	rep movsd	; copy doublewords at DS:ESI to ES:EDI
memcopyDWORDskip:
	pop ecx		; restore the remainder in ECX
	test ecx,ecx	; if count is 0, exit
	jz memcopyEnd
	rep movsb	; copy bytes at DS:ESI to ES:EDI
memcopyEnd:
	popad	; Destory stack frame
	mov esp,ebp
	pop ebp
	ret

Setup16BitSegments:
	push ebp
	mov ebp,esp
	pushad
	mov edx,[ebp+12]	; Get address of the load module
	mov ebx,[ebp+8]		; Get address of the InfoTable
	mov eax,[ebx+14]	; Get base of GDT
	mov ebx,eax
; CS16 is at offset 0x20
	add ebx,0x20
	mov [ebx + 2], dx	; Move base[0..15] to CS16 entry
	shr edx,16
	mov [ebx + 4], dl	; Move base[16..23] to CS16 entry
	mov [ebx + 7], dh	; Move base[24..31] to CS16 entry
; Copy CS16 to DS16 entry
	mov eax,[ebx]
	mov [ebx + 8], eax
	mov eax,[ebx + 4]
	mov [ebx + 12], eax
; Change the DS16 (which is same as CS16 right now to data segment by changing flags)
	mov ax, [ebx + 12]	; change flags
	and ax, 0x00ff		; Preserve base[16..23]
	or ax, 0x9200
	mov [ebx + 12], ax
	mov edx,[ebp+12]	; Get address of the load module
	mov eax,edx			; Separate in segment:offset address
	and dx,0x000f
	and ax,0xfff0
	shr ax,4
	mov [k64load_segment],ax
	mov [k64load_offset],dx
	mov edx,[ebp+16]	; Get DAPkernel address
	mov eax,edx			; Separate in segment:offset address
	and dx,0x000f
	and ax,0xfff0
	shr ax,4
	mov [dap_kernel_segment],ax
	mov [dap_kernel_offset],dx
	mov cx,[ebp+20]
	mov [disk_number],cx
	popad
	mov esp,ebp
	pop ebp
	ret

LoadKernelELFSectors:
	pushad
	mov ax, [k64load_segment]
	shl eax,16
	mov ax, [k64load_offset]
	mov bx, [dap_kernel_segment]
	shl ebx,16
	mov bx, [dap_kernel_offset]
	xor ecx,ecx
	mov cx,[disk_number]
	mov edx, ReturnFrom16BitSegment
	jmp word 0x20:0x0	; Jump to the k64load module
ReturnFrom16BitSegment:
	popad
	ret

JumpToKernel:
	push ebp
	mov ebp,esp
	mov eax,[ebp+8]		; Get PML4T address in eax
	mov cr3,eax			; Set CR3 to PML4T address
	mov eax,cr4
	or eax, 1<<5		; Set PAE enable bit
	mov cr4,eax
	mov ecx, 0xC0000080	; Copy contents of EFER MSR in eax
	rdmsr
	or eax, 1<<8		; Set LM (long mode) bit
	wrmsr				; Write back to EFER MSR
	mov eax,cr0			; Enable paging
	or eax, 1<<31
	mov cr0,eax
	cli					; Disable interrupts. Although interrupts have been disabled till now, one must disable them just to be sure
	mov edi, [ebp+12]
	jmp 0x30:0x80000000		; 0x30 is 64-bit code segment
	; Code beyond this should never get executed
	mov esp,ebp
	pop ebp
	ret