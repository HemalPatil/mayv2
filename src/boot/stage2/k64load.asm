[bits 16]
[org 0x0000]

jmp start

times 8 - ($-$$) db 0

magic_bytes db 'K64L'
pmode_cr0 dd 0
k64load_offset dw 0
k64load_segment dw 0
stackpointer dd 0	; esp from 32-bit protected mode is stored here
test_str db 'It works 16 bit!',0

start:
	; still in protected mode (segment is 16-bit)
	cli
	mov cx,ax		; Store k64load_segment in cx
	mov eax,esp
	mov [stackpointer],eax
	mov ax,0x28	; Segment selector of DS16
	mov ds,ax
	mov es,ax
	mov fs,ax
	mov gs,ax
	mov ss,ax
	mov [k64load_segment],cx	; Store the segment of this module
	add bx, bit16_real			; Store the offset of this module
	mov [k64load_offset],bx
	mov [bit32_return],edx		; The offset of the 32-bit protected mode LOADER32 where this module must return
	mov eax,cr0		; Save protected mode cr0
	mov [pmode_cr0],eax	; and disable paging and protected mode
	and eax,0x7ffffffe
	mov cr0,eax
	jmp far [k64load_offset]	; Do far jump to address stored in segment:offset (little endian)
								; at k64load_offset, so that we shift from 16-bit protected mode
								; to 16-bit real mode

ReturnTo32BitProtected:
	mov eax,cr0		; Enable protected mode
	or ax,1
	mov cr0,eax
	mov ax,0x10		; Setup the data segments with DS32
	mov ds,ax
	mov es,ax
	mov fs,ax
	mov gs,ax
	mov ss,ax
	mov eax,[stackpointer]	; Restore the stack pointer of 32-bit code
	mov esp,eax
	db 0x66		; translates to jmp dword 0x0008:(return address of 32-bit code)
	db 0xEA
bit32_return dd 0
	dw 0x0008

times 1024 - ($-$$) db 0

bit16_real:
	cli
	mov ax,cs
	mov ds,ax
	mov ax,0x0700
	mov ss,ax
	xor esp,esp
	mov sp,0x0c00
	mov si, test_str
	int 22h
	jmp ReturnTo32BitProtected