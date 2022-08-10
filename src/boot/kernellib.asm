[bits 64]

; Assembly level routines

section .text
	global HangSystem
	global GetPhysicalAddressLimit
	global GetLinearAddressLimit
	global SearchRSDP
	global TerminalClearScreenASM

SearchRSDP:
	; Move 'RSD PTR ' magic string into rax in little endian
	; Scan the entire 1st MiB for this magic string
	mov rax, 0x2052545020445352
	mov rdx, 0
SearchRSDPLoop:
	cmp rax, [rdx]
	je SearchRSDPFound
	add rdx, 8
	cmp rdx, 0x100000
	jne SearchRSDPLoop
SearchRSDPNotFound:
	mov rax, 0
	ret
SearchRSDPFound:
	mov rax, rdx
	ret

TerminalClearScreenASM:
	push rax
	push rdi
	push rcx
	mov edi, 0xb8000
	mov rcx, 0x1f4
	mov rax, 0x0f200f200f200f20
	rep stosq
	pop rcx
	pop rdi
	pop rax
	ret

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