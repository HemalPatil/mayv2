[bits 16]
[org 0x0000]

jmp start

times 8 - ($-$$) db 0

magic_bytes db 'K64L'
pmode_cr0 dd 0
k64load_offset dw 0
k64load_segment dw 0
dap_kernel_offset dw 0
dap_kernel_segment dw 0
disk_number dw 0
stackpointer dd 0	; esp from 32-bit protected mode is stored here

start:
	; still in protected mode (segment is 16-bit)
	cli
	mov esi, eax		; Store k64load_segment and k64load_offset in esi
	mov eax, esp
	mov [stackpointer], eax
	mov ax, 0x28	; Segment selector of DS16
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
	add si, bit16_real	; Add offset of the 16 bit real mode code to the offset of this module
	mov [k64load_offset], esi
	mov [disk_number], cx
	mov [dap_kernel_offset], ebx
	mov [bit32_return], edx		; The offset of the 32-bit protected mode LOADER32 where this module must return
	mov eax, cr0		; Save protected mode cr0
	mov [pmode_cr0], eax	; and disable paging and protected mode
	and eax, 0x7ffffffe
	mov cr0, eax
	jmp far [k64load_offset]	; Do far jump to address stored in segment:offset (little endian)
								; at k64load_offset, so that we shift from 16-bit protected mode
								; to 16-bit real mode

ReturnTo32BitProtected:
	mov eax, cr0		; Enable protected mode
	or ax, 1
	mov cr0, eax
	mov ax, 0x10		; Setup the data segments with DS32
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
	mov eax, [stackpointer]	; Restore the stack pointer of 32-bit code
	mov esp, eax
	db 0x66		; translates to jmp dword 0x0008:(return address of 32-bit code)
	db 0xEA
bit32_return dd 0
	dw 0x0008

times 256 - ($-$$) db 0

bit16_real:
	cli
	mov ax, cs	; Make ds = cs
	mov ds, ax
	mov ax, 0x0700	; Setup to the stack same as the one we used in bootload.asm
	mov ss, ax
	xor esp, esp
	mov sp, 0x0c00
	push ds		; We need to store the ds since it will be required later
	xor edx, edx
	mov dl, [disk_number]	; Load the boot disk number
	xor esi, esi
	mov si, [dap_kernel_offset]
	mov ax, [dap_kernel_segment]		; Copy the segment in ds after all operations requiring ds are over
	mov ds, ax
	xor eax, eax
	mov ah, 0x42		; Extended Function 42h, read extended sectors using LBA
	int 13h		; BIOS disk service int 13h
	pop ds	; Restore ds
	jmp ReturnTo32BitProtected