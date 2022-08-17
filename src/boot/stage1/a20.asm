[bits 16]
[org 0x0000]
jmp start

times 8 - ($-$$) db 0

magicBytes db 'A20', 0
a20Msg db 'Enabling A20 gate', 13, 10, 0
a20EnabledMsg db 'A20 gate enabled', 13, 10, 13, 10, 0
a20FailedMsg db 'A20 gate enable failed. Cannot boot!', 13, 10, 0

start:
	push ds						; ds is going to be trashed so save it

	mov ax, cs					; set data segment as current code segment
	mov ds, ax
	mov si, a20Msg
	int 0x22

	call checkA20				; check if A20 line is already enabled
	cmp ax, 1					; ax=1 means line enabled
	je a20Enabled

	mov ax, 0x2401				; Use BIOS method to activate A20 line
	int 0x15

	call checkA20				; check if A20 enabled now
	cmp ax, 1
	je a20Enabled

	mov si, a20FailedMsg		; if not enabled yet, halt the processor completely
	int 0x22
errorEnd:
	cli
	hlt
	jmp errorEnd

a20Enabled:
	mov si, a20EnabledMsg
	int 0x22

end:
	pop ds
	retf

checkA20:
	push ds
	push es

	xor ax, ax		; ax=0x0000
	mov ds, ax
	not ax			; ax=0xffff
	mov es, ax
	mov ax, cs		; ax=cs
	shl ax, 4		; ax=absolute address of current segment 
	mov si, ax
	mov di, ax
	add si, 0x08
	add di, 0x18
	
	mov ax, [ds:si]
	mov bx, [es:di]
	push ax
	push bx

	mov word [ds:si], 0xdead
	mov word [es:di], 0xbeef
	cmp word [ds:si], 0xbeef

	pop bx
	pop ax
	mov [ds:si], ax
	mov [es:di], bx

	mov ax, 0
	je checkA20End

	mov ax, 1

checkA20End:
	pop es
	pop ds
	ret
