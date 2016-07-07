[bits 16]
[org 0x0000]
jmp start

times 8 - ($-$$) db 0

magic_bytes db 'A20',0
a20_msg db 'Enabling A20 gate',13,10,0
a20_enabled_msg db 'A20 gate enabled',13,10,13,10,0
a20_failed_msg db 'A20 gate enable failed. Cannot boot!',13,10,0

start:
	push ds						; ds is going to be trashed so save it

	mov ax,cs					; set data segment as current code segment
	mov ds,ax
	mov si,a20_msg
	int 22h

	call check_a20				; check if A20 line is already enabled
	cmp ax,1					; ax=1 means line enabled
	je a20_enabled

	mov ax,0x2401				; Use BIOS method to activate A20 line
	int 15h

	call check_a20				; check if A20 enabled now
	cmp ax,1
	je a20_enabled

	mov si,a20_failed_msg		; if not enabled yet, halt the processor completely
	int 22h
error_end:
	cli
	hlt
	jmp error_end

a20_enabled:
	mov si,a20_enabled_msg
	int 22h

end:
	pop ds
	retf

check_a20:
	push ds
	push es

	xor ax,ax		;ax=0x0000
	mov ds,ax
	not ax			;ax=0xffff
	mov es,ax
	mov ax,cs		;ax=cs
	shl ax,4		;ax=absolute address of current segment 
	mov si,ax
	mov di,ax
	add si,0x08
	add di,0x18
	
	mov ax,[ds:si]
	mov bx,[es:di]
	push ax
	push bx

	mov word [ds:si], 0xdead
	mov word [es:di], 0xbeef
	cmp word [ds:si],0xbeef

	pop bx
	pop ax
	mov [ds:si],ax
	mov [es:di],bx

	mov ax,0
	je check_a20_end

	mov ax,1

check_a20_end:
	pop es
	pop ds
	ret
