[bits 32]

KERNEL_LOWERHALF_ORIGIN equ 0x80000000
LOADER_STACK_SIZE equ 4096

section .data
k64LoadOffset dw 0
k64LoadSegment dw 0
dapKernelOffset dw 0
dapKernelSegment dw 0
diskNumber dw 0

section .rodata
	global hexSpace
	; A copy of the initial GDT from kernel is
	; here because doing far jumps in 64-bit mode is tedious
	; so to change the cs selector, a copy of GDT will be loaded from here itself
	; do a far jump and then just change base of GDT to a 64-bit address
kernelGdtCopy:
	; null entry
	dq 0
	; 64 bit code segment
	dq 0x00209a0000000000
	; 64 bit data segment
	dq 0x0020920000000000
gdtDescriptor:
	dw $ - kernelGdtCopy -1
	dd kernelGdtCopy
hexSpace db '0123456789ABCDEF', 0

section .bss
	global loaderStack
loaderStack:
	resb LOADER_STACK_SIZE

section .setup
	global loader32Start
	extern loader32Main
loader32Start:
	cli
	mov ebp, 0x7bf0		; Point ebp to parameters that were passed on the stack at 0x7c00
	mov esp, loaderStack + LOADER_STACK_SIZE		; Setup LOADER32 stack
	mov eax, [ebp + 12]		; 4 32-bit parameters were passed
	push eax
	mov eax, [ebp + 8]
	push eax
	mov eax, [ebp + 4]
	push eax
	mov eax, [ebp]
	push eax
	call loader32Main
	; code beyond this will not get executed unless there was an error during kernel initialization
loader32End:
	cli
	hlt
	jmp loader32End

section .text
	global memset
	global memcpy
	global printHex
	global swap
	global getPhysicalAddressLimit
	global getLinearAddressLimit
	global setup16BitSegments
	global loadKernel64ElfSectors
	global jumpToKernel64
	extern printChar
printHex:
	push ebp
	mov ebp, esp
	pushad
	mov ecx, [ebp + 12]
	mov esi, [ebp + 8]
	add esi, ecx
	dec esi
printHexLoop:
	push ecx
	push esi
	mov dl, [esi]
	call printHexCore
	pop esi
	dec esi
	pop ecx
	loop printHexLoop
	popad
	mov esp, ebp
	pop ebp
	ret

printHexCore:
	pushad
	push edx
	mov ebx, hexSpace
	xor eax, eax
	mov al, dl
	shr al, 4
	xlat
	push 0x0f
	push eax
	call printChar
	add esp, 8
	pop edx
	mov ebx, hexSpace
	xor eax, eax
	mov al, dl
	and al, 0x0f
	xlat
	push 0x0f
	push eax
	call printChar
	add esp, 8
	popad
	ret
	
swap:
	push ebp
	mov ebp, esp
	pushad
	mov esi, [ebp + 8]
	mov edi, [ebp + 12]
	mov eax, [ebp + 16]
	test eax, eax
	jz swapEnd
	xor edx, edx
	mov ecx, 4
	div ecx
	push edx
	test eax, eax
	jz swapDwordSkip
	mov ecx, eax
swapDwordLoop:
	mov eax, [esi]
	xchg eax, [edi]
	mov [esi], eax
	add esi, 4
	add edi, 4
	loop swapDwordLoop
swapDwordSkip:
	pop ecx
	test ecx, ecx
	jz swapEnd
swapByteLoop:
	mov al, [esi]
	xchg al, [edi]
	mov [esi], al
	inc esi
	inc edi
	loop swapByteLoop
swapEnd:	
	popad
	mov esp, ebp
	pop ebp
	ret

getPhysicalAddressLimit:
	mov eax, 0x80000008
	cpuid
	and eax, 0xff
	ret

getLinearAddressLimit:
	mov eax, 0x80000008
	cpuid
	shr eax, 8
	and eax, 0xff
	ret

memset:
	push ebp
	mov ebp, esp
	pushad
	mov edi, [ebp + 8]
	mov eax, [ebp + 16]
	test eax, eax
	jz memsetEnd
	xor edx, edx
	mov ecx, 4
	div ecx
	push edx
	test eax, eax
	jz memsetDwordSkip
	push eax
	mov ecx, 4
memsetEaxLoop:
	shl eax, 8
	mov al, [ebp + 12]
	loop memsetEaxLoop
	pop ecx
	rep stosd
memsetDwordSkip:
	pop ecx
	test ecx, ecx
	jz memsetEnd
	mov al, [ebp + 12]
	rep stosb
memsetEnd:
	popad
	mov esp, ebp
	pop ebp
	ret

memcpy:
	push ebp	; Create stack frame
	mov ebp, esp
	pushad
	mov ax, ds	; Copy ds to es
	mov es, ax
	mov esi, [ebp + 8]		; Get source pointer in ESI
	mov edi, [ebp + 12]	; Get destination pointer in EDI
	mov eax, [ebp + 16]	; Get count in EAX
	test eax, eax		; If count is 0, exit
	jz memcpyEnd
	xor edx, edx		; Divide count by 4
	mov ecx, 4
	div ecx
	push edx	; Save the remainder
	test eax, eax	; Check if quotient is 0, if 0 then skip DWORD loop
	jz memcpyDwordSkip
	mov ecx, eax		; copy quotient to ECX
	rep movsd	; copy doublewords at DS:ESI to ES:EDI
memcpyDwordSkip:
	pop ecx		; restore the remainder in ECX
	test ecx, ecx	; if count is 0, exit
	jz memcpyEnd
	rep movsb	; copy bytes at DS:ESI to ES:EDI
memcpyEnd:
	popad	; Destory stack frame
	mov esp, ebp
	pop ebp
	ret

setup16BitSegments:
	push ebp
	mov ebp, esp
	pushad
	mov edx, [ebp + 12]	; Get address of the load module
	mov ebx, [ebp + 8]		; Get address of the InfoTable
	mov eax, [ebx + 14]	; Get base of GDT
	mov ebx, eax
; CS16 is at offset 0x20
	add ebx, 0x20
	mov [ebx + 2], dx	; Move base[0..15] to CS16 entry
	shr edx, 16
	mov [ebx + 4], dl	; Move base[16..23] to CS16 entry
	mov [ebx + 7], dh	; Move base[24..31] to CS16 entry
; Copy CS16 to DS16 entry
	mov eax, [ebx]
	mov [ebx + 8], eax
	mov eax, [ebx + 4]
	mov [ebx + 12], eax
; Change the DS16 (which is same as CS16 right now to data segment by changing flags)
	mov ax, [ebx + 12]	; change flags
	and ax, 0x00ff		; Preserve base[16..23]
	or ax, 0x9200
	mov [ebx + 12], ax
	mov edx, [ebp + 12]	; Get address of the load module
	mov eax, edx			; Separate in segment:offset address
	and dx, 0x000f
	and ax, 0xfff0
	shr ax, 4
	mov [k64LoadSegment], ax
	mov [k64LoadOffset], dx
	mov edx, [ebp + 16]	; Get DAPkernel address
	mov eax, edx			; Separate in segment:offset address
	and dx, 0x000f
	and ax, 0xfff0
	shr ax, 4
	mov [dapKernelSegment], ax
	mov [dapKernelOffset], dx
	mov cx, [ebp + 20]
	mov [diskNumber], cx
	popad
	mov esp, ebp
	pop ebp
	ret

loadKernel64ElfSectors:
	pushad
	mov ax, [k64LoadSegment]
	shl eax, 16
	mov ax, [k64LoadOffset]
	mov bx, [dapKernelSegment]
	shl ebx, 16
	mov bx, [dapKernelOffset]
	xor ecx, ecx
	mov cx, [diskNumber]
	mov edx, returnFrom16BitSegment
	jmp word 0x20:0x0	; Jump to the k64load module
returnFrom16BitSegment:
	popad
	ret

jumpToKernel64:
	push ebp
	mov ebp, esp
	mov eax, [ebp + 8]		; Get PML4T address in eax
	mov cr3, eax			; Set CR3 to PML4T address
	mov eax, cr4
	or eax, 1 << 5		; Set PAE enable bit
	mov cr4, eax
	mov ecx, 0xc0000080	; Copy contents of EFER MSR in eax
	rdmsr
	or eax, 1 << 8		; Set LM (long mode) bit
	wrmsr				; Write back to EFER MSR
	mov eax, cr0			; Enable paging
	or eax, 1 << 31
	mov cr0, eax
	cli
	mov edi, [ebp + 12]	; Load edi/rdi with info table address, which is the 1st parameter passed to kernelMain
	mov esi, [ebp + 16]	; Load esi/rsi with kernel's lower half size
	mov edx, [ebp + 20]	; Load edx/rdx with kernel's higher half size
	mov ecx, [ebp + 24]	; Load edx/rdx with usable memory address right after PML4 pages
	lgdt [gdtDescriptor]	; shift to kernel GDT
	; Jump to lower half entry point of the kernel
	jmp 0x8:KERNEL_LOWERHALF_ORIGIN
	; Code beyond this should never get executed
	mov esp, ebp
	pop ebp
	ret
