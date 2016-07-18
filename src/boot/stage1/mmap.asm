[bits 16]
[org 0x0000]
jmp start

times 8 - ($-$$) db 0

magic_bytes db 'MMAP'
mmap_msg db 'Generating Memory Map',13,10,0
mmap_done_msg db 'Memory Map complete',13,10,13,10,0
mmap_fail_msg db 'Memory Map failed. ',0
cannot_boot db 'Cannot boot! ',13,10,0
insuff_mem db 'Insufficient memory detetcted. ',0
num1_low dd 0
num1_high dd 0
num2_low dd 0
num2_high dd 0
valid dw 0

start:
	push bp
	mov bp,sp
	pusha
	push ds
	mov ax,cs
	mov ds,ax

	mov si, mmap_msg					; Generating memory map entries
	int 22h

	mov ax,[bp+8]
	mov es,ax
	mov bx,[bp+6]
	mov word [es:bx + 4],0x0000				; Store address of MMAP entries as second enttry in our custom table
	mov word [es:bx + 6],0x0060				; in segment:offset little endian format

	push bp
	
	;Set up the memory map with int 0x15 //Courtesy of osdev.org wiki article 'Detecting Memory'
mmap:
	mov bx,0x0060
	mov es,bx
	xor ebx, ebx					; ebx must be 0 to start
	xor bp, bp						; keep an entry count in bp
	mov edx, 0x0534D4150			; Place "SMAP" into edx
	mov eax, 0xe820
	xor di,di
	mov [es:di + 20], dword 1		; force a valid ACPI 3.X entry
	mov ecx, 24						; ask for 24 bytes
	int 0x15
	jc mmap_fail				; carry set on first call means "unsupported function"
	mov edx, 0x0534D4150			; Some BIOSes apparently trash this register?
	cmp eax, edx					; on success, eax must have been reset to "SMAP"
	jne mmap_fail
	test ebx, ebx					; ebx = 0 implies list is only 1 entry long (worthless)
	je mmap_fail
	jmp mmap_inside
mmap_loop:
	mov eax, 0xe820					; eax, ecx get trashed on every int 0x15 call
	mov [es:di + 20], dword 1		; force a valid ACPI 3.X entry
	mov ecx, 24						; ask for 24 bytes again
	int 0x15
	jc mmap_exit				; carry set means "end of list already reached"
	mov edx, 0x0534D4150			; repair potentially trashed register
mmap_inside:
	cmp cx,0					; skip any 0 length entries
	je mmap_skip
	cmp cl, 20						; got a 24 byte ACPI 3.X response?
	jbe short mmap_short
	test byte [es:di + 20], 1		; if so: is the "ignore this data" bit clear?
	je short mmap_skip
mmap_short:
	mov ecx, [es:di + 8]			; get lower uint32_t of memory region length
	or ecx, [es:di + 12]			; "or" it with upper uint32_t to test for zero
	jz mmap_skip					; if length uint64_t is 0, skip entry
	cmp word [valid],0
	jne mmap_valid_entry
	cmp byte [es:di + 16],1		; Check if memory region type is 1, i.e. free for use
	jne mmap_valid_entry		; If not skip this validation of this entry
	push eax
	mov eax,[es:di]				; Check if the memory 0x80000 to 0x90000
	mov [num1_low],eax			; is available in the same memory region
	mov eax,[es:di+4]			; and the memory is usable
	mov [num1_high],eax
	xor eax,eax
	mov [num2_high],eax
	mov dword [num2_low], 0x80000
	pop eax
	call cmp_uint64_t
	ja mmap_valid_entry
	push eax
	mov eax,[es:di+8]
	mov [num2_low],eax
	mov eax,[es:di+12]
	mov [num2_high],eax
	call add_uint64_t
	xor eax,eax
	mov [num2_high],eax
	mov eax,0x90000
	mov [num2_low],eax
	pop eax
	call cmp_uint64_t
	jb mmap_valid_entry
	mov word [valid],1
mmap_valid_entry:
	inc bp							; got a good entry: ++count, move to next storage spot
	add di, 24
mmap_skip:
	test ebx, ebx					; if ebx resets to 0, list is complete
	jne mmap_loop
mmap_exit:
	mov cx,bp
	pop bp
	mov bx,[bp+8]
	mov es,bx
	mov bx,[bp+6]
	mov [es:bx + 2], cx					; store the entry count in our custom table
	mov si, mmap_done_msg			; Show mmap complete message
	int 22h
	clc								; there is "jc" on end of list to this point, so the carry must be cleared
	jmp mmap_finish
mmap_fail:
	mov si, mmap_fail_msg			; Show mmap fail message
	int 22h
	mov si, cannot_boot
	int 22h
	stc								; "function unsupported" error exit
error_end:
	cli
	hlt
	jmp error_end
mmap_finish:
	cmp word [valid],1
	jne insuffmem_msg

	;Check memory size (We need atleast 1GiB of free usable conventional memory)
	mov bx,[bp+8]					;Custom table stored at 0x0050:0x0000
	mov es,bx
	mov bx,[bp+6]
	mov cx,[es:bx + 2]				; Get count of MMAP entries
	mov di,[es:bx + 4]				; Get offset and segment of MMAP entries from custom table
	mov ax,[es:bx + 6]
	mov es,ax
	xor eax,eax			; Store length in eax
	mov edx,0x40000000		; Put 1GiB in edx
memcount_loop:
	cmp byte [es:di+16],1		; Check memory type (should be 1 for conventional memory)
	jne memcount_skip		; If not 1 then skip this entry
	mov ebx, [es:di+12]		; Check higher 32 bits of uint64_t length
	test ebx,ebx			; If not zero then this entry is greater than 1 GiB
	jnz greater_than_1GiB
	mov ebx, [es:di+8]		; Check if lower 32 bits of uint64_t length
	cmp ebx,edx			; is greater than 1GiB
	jnb greater_than_1GiB
	add eax,ebx			; Add the length of this region to mem count so far
	jc greater_than_1GiB		; If overflows, then greater than 1GiB
memcount_skip:
	add di,24		; Move to next entry
	loop memcount_loop

insuffmem_msg:
	mov si,insuff_mem				; Insufficient memory found in the system
	int 22h							; Halt processor completely
	mov si,cannot_boot
	int 22h
insufficient_memory:
	cli
	hlt
	jmp insufficient_memory

greater_than_1GiB:
	pop ds
	popa
	mov sp,bp
	pop bp
	retf 4

add_uint64_t:
	push eax		; Add lower 32 bits and then add higher 32 bits along with carry
	mov eax,[num1_low]
	add eax,[num2_low]
	mov [num1_low],eax
	mov eax,[num1_high]
	adc eax,[num2_high]
	mov [num1_high],eax
	pop eax
	ret

cmp_uint64_t:
	push eax		; Compare higher 32 bits
	mov eax,[num1_high]	; if they are same, compare lower 32 bits
	cmp eax,[num2_high]
	jne cmp_end
	mov eax,[num1_low]
	cmp eax,[num2_low]
cmp_end:
	pop eax
	ret
