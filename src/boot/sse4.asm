[bits 64]

section .data
	global floatSaveRegion
align 16
floatSaveRegion:
	times 512 db 0

section .text
	global enableSse4
enableSse4:
	push rbx	; Preserve rbx to stay compatible with System V ABI
	mov eax, 1
	cpuid
	; Check CLFLUSH
	mov eax, edx
	shr eax, 19
	and eax, 1
	jz noSse4
	; Check FXSAVE, FXRESTOR
	mov eax, edx
	shr eax, 24
	and eax, 1
	jz noSse4
	; Check SSE
	mov  eax, edx
	shr eax, 25
	and eax, 1
	jz noSse4
	; Check SSE2
	mov  eax, edx
	shr eax, 26
	and eax, 1
	jz noSse4
	; Check SSE3
	mov eax, ecx
	and eax, 1
	jz noSse4
	; Check SSSE3
	mov eax, ecx
	shr eax, 9
	and eax, 1
	jz noSse4
	; Check SSE4.1
	mov eax, ecx
	shr eax, 19
	and eax, 1
	jz noSse4
	; Check SSE4.2
	mov eax, ecx
	shr eax, 20
	and eax, 1
	jz noSse4
	mov rax, cr0
	and ax, 0xfffb		; clear coprocessor emulation CR0.EM
	or ax, 0x2			; set coprocessor monitoring CR0.MP
	mov cr0, rax
	mov rax, cr4
	or ax, 3 << 9		; set CR4.OSFXSR and CR4.OSXMMEXCPT at the same time
	mov cr4, rax
	pop rbx
	mov rax, 1
	ret
noSse4:
	; TODO: show some error message. Cannot use terminal functions
	; since they rely on optimizations
	cli
	hlt
