[bits 16]
[org 0x0000]
jmp start

times 8 - ($-$$) db 0

magic_bytes db 'GDT', 0
gdt_msg db 'Setting up GDT', 13, 10, 13, 10, 0

TSS:
	prev_tss dd 0
	esp0 dd 0x0009000
	ss0 dd 0x00000010
	esp1 dd 0
	ss1 dd 0
	esp2 dd 0
	ss2 dd 0
	tcr3 dd 0
	teip dd 0
	teflags dd 0
	teax dd 0
	tecx dd 0
	tedx dd 0
	tebx dd 0
	tesp dd 0
	tebp dd 0
	tesi dd 0
	tedi dd 0
	tes dd 0
	tcs dd 0
	tss dd 0
	tds dd 0
	tfs dd 0
	tgs dd 0
	tldt dd 0
	ttrap dw 0
	tiomap_base dw 0

tss_base dd 0
tss_limit dd 104

start:
	push bp
	mov bp, sp
	pusha
	push ds

	mov ax, cs
	mov ds, ax
	mov si, gdt_msg
	int 22h

;segment selector 0x0
	mov bx, 0x1000
	mov es, bx
	xor bx, bx
	mov dword [es:bx], 0				; Null entry 8 bytes
	mov dword [es:bx + 4], 0

;segment selector 0x08
	add bx, 8							; cs entry
	mov dword [es:bx], 0xffff			; cs limit[0..15](low), base[0..15](high)
	mov dword [es:bx + 4], 0x00cf9b00	; cs base[16..23](low8), accessflags, lim[16..19], flags, base[24..31](high)

;segment selector 0x10
	add bx, 8							; ds entry
	mov dword [es:bx], 0xffff			; ds limit[0..15](low), base[0..15](high)
	mov dword [es:bx + 4], 0x00cf9200	; ds base[16..23](low8), access flags, lim[16..19], flags, base[24..31](high)

	xor eax, eax
	mov ax, ds
	shl eax, 4
	add eax, TSS
	mov [tss_base], eax
	mov eax, [tss_limit]

;segment selector 0x18
	add bx, 8						; TSS entry
	mov [es:bx], ax					; tss limit[0..15]
	mov ax, [tss_base]
	mov [es:bx + 2], ax				; tss base[0..15]
	mov al, [tss_base + 2]
	mov [es:bx + 4], al				; tss base[16..23]
	mov byte [es:bx + 5], 10001001b	; tss access flags
	mov al, [tss_limit + 2]
	or al, 01000000b 
	mov [es:bx + 6], al				; tss flags (high), lim[16..19](low)
	mov al, [tss_base + 3]
	mov [es:bx + 7], al				; tss base[24..31]

	;The base for cs16 and ds16 protected mode segments need to be changed from LOADER32
	;since we don't know as of yet where the code is loaded
;segment selector 0x20
	add bx, 8							; 16-bit code segment for jump to real mode back to set videomode
	mov dword [es:bx], 0xffff			; cs limit[0..15](low), base[0..15](high)
	mov dword [es:bx + 4], 0x9a00		; cs base[16..23](low8), accessflags, lim[16..19], flags, base[24..31](high)

;segment selector 0x28
	add bx, 8							; 16-bit data segment for jump to real mode back to set videomode
	mov dword [es:bx], 0xffff			; ds limit[0..15](low), base[0..15](high)
	mov dword [es:bx + 4], 0x9200		; ds base[16..23](low8), access flags, lim[16..19], flags, base[24..31](high)

	mov bx, [bp+8]					; GDT descriptor is in our custom table at offset 12
	mov es, bx
	mov bx, [bp+6]
	add bx, 12
	mov word [es:bx], 47				; 6 entries as of now (null, cs32, ds32, tss, cs16, ds16)
	mov dword [es:bx + 2], 0x00010000		; our GDT is at 0x00010000 to 0x00020000

	pop ds
	popa
	mov sp, bp
	pop bp
	retf 4
