[bits 16]
[org 0x0000]
jmp start

VBE_GET_CONTROLLER_INFO equ 0x4f00
VBE_GET_MODE_INFO equ 0x4f01
VBE_SUCCESS equ 0x004f
VBE_VESA_SIGNATURE equ 0x41534556
VBE_MODES_INFO_LOCATION equ 0x30000
VBE_MODE_INFO_SIZE equ 256
VBE_INVALID_MODE equ 0xffff
INFOTABLE_VBE_MODES_COUNT equ 22
INFOTABLE_VBE_MODES_INFO_LOCATION equ 0x30000
INFOTABLE_VBE_MODE_NUMBERS_LOCATION equ 40

times 8 - ($-$$) db 0

magicBytes db 'VIDM'
vidModesStr db 'Getting VESA VBE modes', 13, 10, 0
infoBlockFailStr db 'Cannot get VESA VBE modes information. Cannot boot!', 13, 10, 0

vbeInfoBlock:
	.signature db 'VBE2'
	.version dw 0
	.oem dd 0
	.capabilities dd 0
	.videoModesOffset dw 0
	.videoModesSegment dw 0
	.videoMemory dw 0
	.revision dw 0
	.vendorOffset dw 0
	.vendorSegment dw 0
	.productNameOffset dw 0
	.productNameSegment dw 0
	times (512 - 30) db 0

start:
	push bp
	mov bp, sp
	pusha
	push ds
	push es
	push di
	mov ax, cs
	mov ds, ax
	mov si, vidModesStr
	int 0x22

	; Get VBE info block into es:di by calling int 0x10; ax=0x4f00
	mov ax, cs
	mov es, ax
	mov di, vbeInfoBlock
	mov ax, VBE_GET_CONTROLLER_INFO
	int 0x10
	cmp ax, VBE_SUCCESS
	jne errorEnd
	mov eax, [vbeInfoBlock]
	cmp eax, VBE_VESA_SIGNATURE
	jne errorEnd

	; Start getting the modes info
	; ds:si points at the modes array, si is incremented by 2 each time as each mode number is 2 bytes in size
	; Keep loading cx with mode number from [ds:si] until VBE_INVALID_MODE is encountered
	; es:di points to where each mode's info is stored, di is incremented by 256 each time i.e. size of mode info struct
	; FIXME: assumes the memory at VBE_MODES_INFO_LOCATION of size 64KiB is free
	; dx holds the mode count
	; store the modes count, numbers, and info location in InfoTable
	mov ax, [vbeInfoBlock.videoModesSegment]
	mov ds, ax
	mov ax, [vbeInfoBlock.videoModesOffset]
	xor esi, esi
	mov si, ax
	xor eax, eax
	mov ax, ds
	shl eax, 4
	add eax, esi
	push eax
	mov ax, VBE_MODES_INFO_LOCATION >> 4
	mov es, ax
	xor di, di
	xor dx, dx
vbeModesLoop:
	mov cx, [ds:si]
	cmp cx, VBE_INVALID_MODE
	je vbeModesLoopEnd
	mov ax, VBE_GET_MODE_INFO
	int 0x10
	cmp ax, VBE_SUCCESS
	jne errorEnd
	inc si
	inc si
	add di, VBE_MODE_INFO_SIZE
	inc dx
	jmp vbeModesLoop
vbeModesLoopEnd:
	mov ax, [bp + 8]
	mov es, ax
	mov bx, [bp + 6]
	mov word [es:bx + INFOTABLE_VBE_MODES_COUNT], dx
	mov dword [es:bx + INFOTABLE_VBE_MODES_INFO_LOCATION], VBE_MODES_INFO_LOCATION
	pop eax
	mov dword [es:bx + INFOTABLE_VBE_MODE_NUMBERS_LOCATION], eax
	mov dword [es:bx + INFOTABLE_VBE_MODE_NUMBERS_LOCATION + 4], 0
	; FIXME: should switch video mode by dropping from long->protected->real mode
	; this is hacky
	; mov ax, 0x4f02
	; mov bx, 0x4144
	; int 0x10
	pop di
	pop es
	pop ds
	popa
	mov sp, bp
	pop bp
	retf 4

errorEnd:
	mov si, infoBlockFailStr
	int 0x22
	cli
	hlt
	jmp errorEnd