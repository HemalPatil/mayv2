[bits 16]
[org 0x0000]

jmp start

times 8- ($-$$) db 0

magic_bytes db 'K64L'
pmode_cr0 dd 0

start:		; still in protected mode (segment is 16-bit)
	cli
	mov eax,cr0		; Save protected mode cr0
	mov [pmode_cr0],eax	; and disable paging and protected mode
	and eax,0x7ffffffe
	mov cr0,eax
