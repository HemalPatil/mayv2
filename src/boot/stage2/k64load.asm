[bits 16]
[org 0x0000]

jmp start

times 8 - ($-$$) db 0

magicBytes db 'K64L'
pmodeCr0 dd 0
k64LoadOffset dw 0
k64LoadSegment dw 0
dapKernelOffset dw 0
dapKernelSegment dw 0
diskNumber dw 0
stackPointer dd 0	; esp from 32-bit protected mode is stored here

start:
	; still in protected mode (segment is 16-bit)
	cli
	mov esi, eax		; Store k64LoadSegment and k64LoadOffset in esi
	mov eax, esp
	mov [stackPointer], eax
	mov ax, 0x28	; Segment selector of DS16
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
	add si, bit16Real	; Add offset of the 16 bit real mode code to the offset of this module
	mov [k64LoadOffset], esi
	mov [diskNumber], cx
	mov [dapKernelOffset], ebx
	mov [bit32Return], edx		; The offset of the 32-bit protected mode LOADER32 where this module must return
	mov eax, cr0		; Save protected mode cr0
	mov [pmodeCr0], eax	; and disable paging and protected mode
	and eax, 0x7ffffffe
	mov cr0, eax
	jmp far [k64LoadOffset]	; Do far jump to address stored in segment:offset (little endian)
								; at k64LoadOffset, so that we shift from 16-bit protected mode
								; to 16-bit real mode

returnTo32BitProtected:
	mov eax, cr0		; Enable protected mode
	or ax, 1
	mov cr0, eax
	mov ax, 0x10		; Setup the data segments with DS32
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
	mov eax, [stackPointer]	; Restore the stack pointer of 32-bit code
	mov esp, eax
	db 0x66		; translates to jmp dword 0x0008:(return address of 32-bit code)
	db 0xEA
bit32Return dd 0
	dw 0x0008

times 256 - ($-$$) db 0

bit16Real:
	cli
	mov ax, cs	; Make ds = cs
	mov ds, ax
	mov ax, 0x0700	; Setup to the stack same as the one we used in bootload.asm
	mov ss, ax
	xor esp, esp
	mov sp, 0x0c00
	push ds		; We need to store the ds since it will be required later
	xor edx, edx
	mov dl, [diskNumber]	; Load the boot disk number
	xor esi, esi
	mov si, [dapKernelOffset]
	mov ax, [dapKernelSegment]		; Copy the segment in ds after all operations requiring ds are over
	mov ds, ax
	xor eax, eax
	mov ah, 0x42		; Extended Function 0x42, read extended sectors using LBA
	int 0x13		; BIOS disk service int 0x13
	pop ds	; Restore ds
	jmp returnTo32BitProtected