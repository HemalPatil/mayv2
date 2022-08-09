; HemalOS mayv2

[bits 16]
[org 0x0000]

;Our bootloader is loaded from an El-Torito standard "no emulation" boot CD

jmp 0x07c0:start	;Ensure cs=0x07c0

times 8 - ($-$$) db 0
print_string:
	pusha
	mov ah, 0x0e				; Setup teletype mode
	cld						; Clear direction flag so that we move towards higher addresses
print_string_loop:
	lodsb					; load character from string at ds:si
	cmp al, 0				; Check if C-style null character
	je print_string_end		; if null character, end the print_string interrupt
	int 0x10				; call int 10 to display character
	jmp print_string_loop	; loop until end of string
print_string_end:
	popa
	iret					; Return from interrupt

times 32 - ($-$$) db 0

dap_x64:						; Disk address packet (DAP)
	db 0x10						; Size of DAP (always 0x10)
	db 0x00						; Always 0x00
	dw 0x0000					; Number of sectors to read
	dw 0x0000					; offset
	dw 0x0000					; segment
	dq 0x00000000				; starting sector number

dap_mmap:
	db 0x10
	db 0x00
	dw 0x0000
	dw 0x0000
	dw 0x0000
	dq 0x00000000

dap_vidmodes:
	db 0x10
	db 0x00
	dw 0x0000
	dw 0x0000
	dw 0x0000
	dq 0x00000000

dap_a20enable:
	db 0x10
	db 0x00
	dw 0x0000
	dw 0x0000
	dw 0x0000
	dq 0x00000000

dap_gdt:
	db 0x10
	db 0x00
	dw 0x0000
	dw 0x0000
	dw 0x0000
	dq 0x00000000

dap_kernel64load:
	db 0x10
	db 0x00
	dw 0x0000
	dw 0x0000
	dw 0x0000
	dq 0x00000000

dap_elfparse:
	db 0x10
	db 0x00
	dw 0x0000
	dw 0x0000
	dw 0x0000
	dq 0x00000000

dap_kernel32elf:
	db 0x10
	db 0x00
	dw 0x0000
	dw 0x0000
	dw 0x0000
	dq 0x00000000

dap_kernel64elf:
	db 0x10
	db 0x00
	dw 0x0000
	dw 0x0000
	dw 0x0000
	dq 0x00000000

KernelActualVirtualMemorySize dq 0

; Magic bytes
x64_magic db 'x64', 0
mmap_magic db 'MMAP'
vidmodes_magic db 'VIDM'
a20enable_magic db 'A20', 0
gdt_magic db 'GDT', 0
kernel64load_magic db 'K64L'
elf32parse_magic db 'ELFP'
kernel32elf_magic db 'K32E'

drive_number dw 0
info_table_offset dw 0
info_table_segment dw 0x50
hello_msg db 'Welcome to Hemal', 39, 's OS! Booting.....', 13, 10, 13, 10, 0
corefiles_error db 'Some core files cannot be found! Cannot boot!', 13, 10, 0
disk_error db 'Boot disk error! Cannot Boot!', 13, 10, 0
boot_end db 'Bootloader end', 13, 10, 0
kernel32_jmp_failed db 'Jumping to kernel failed. Cannot boot!', 13, 10, 0
returnx64 db 'returned form x64.bin', 13, 10, 0

start:
	mov ax, 0x07c0	;Right now DS=CS
	mov ds, ax		;Setup DS

	cli						; Disable interrupts
	mov ax, [info_table_segment]
	mov es, ax
	mov [drive_number], dl	; Store the disk number from which this bootloader was loaded
	mov bx, [info_table_offset]
	xor dh, dh
	mov [es:bx], dx			; Store the disk number from which this bootloader was loaded in our custom table

	xor ax, ax							; Setup int 22h
	mov es, ax
	mov word [es:136], print_string		; Offset in the current segment
	mov word [es:138], cs				; Code segment is current code segment

	mov ax, 0x0700						; Set up 3 KiB stack space before bootloader
	mov ss, ax							; starting from 0x7000 to 0x7c00
	mov sp, 0x0c00
	sti					; Enable interrupts

	mov ax, 0x0003				; Set 80x25 text mode
	int 10h

	mov si, hello_msg					; Show hello world message on screen

	mov bx, [info_table_segment]		; Store KernelActualVirtualMemorySize in Info Table
	mov es, bx
	mov bx, [info_table_offset]
	add bx, 0x18
	mov eax, [KernelActualVirtualMemorySize]
	mov [es:bx], eax
	mov eax, [KernelActualVirtualMemorySize + 4]
	mov [es:bx + 4], eax

	mov ah, 41h						; Check disk extensions
	int 13h
	jc diskerror						; No support for disk extensions if carry flag set

	mov cx, 8						; Load 8 core files (subject to change if more files are needed to kick start kernel64)
	mov si, dap_x64						; Load address of the first entry
	mov di, x64_magic					; Load address of the first magic number
	mov dl, [drive_number]					; Boot drive remains consistent for all drive operations

loadcorefiles_loop:
	mov ah, 42h						; Extended function 42h of int 13h
	push cx
	mov cx, 3						; Try reading correct data 3 times
retry_loop:
	pusha
	int 13h							; Load data from disk
	popa
	mov ax, [si+6]						; Get segment
	mov es, ax
	mov bx, [si+4]						; Get offset
	mov eax, [es:bx + 8]					; Get magic bytes from loaded data at offset 8
	mov ebx, [di]
	cmp ebx, 'K32E'						; Check if it is our kernel32.elf file
	jne loadcorefiles_cont1
	mov bx, [si+4]
	mov eax, [es:bx]						; First 4 bytes of an elf are magic bytes 0x7f, 'ELF'
	mov ebx, 0x464C457F					; Put those magic bytes in ebx in proper little endian format
loadcorefiles_cont1:
	cmp eax, ebx							; Compare magic bytes
	je loadcorefiles_cont
	dec cx
	jnz retry_loop
	jmp diskerror
loadcorefiles_cont:
	pop cx
	add si, 16							; Go to next DAP
	add di, 4
	loop loadcorefiles_loop

	call far [dap_x64+4]
	
	push dword [info_table_offset]
	call far [dap_mmap+4]				; Generate memory map

	;push dword [info_table_offset]
	;call far [dap_vidmodes+4]			; Get list of all video modes
	
	call far [dap_a20enable+4]			; Enable the A20 line to access above 1MiB

	push dword [info_table_offset]
	call far [dap_gdt+4]				; Setup GDT

	mov sp, 0x0c00

	xor eax, eax
	mov ax, [dap_kernel64load + 6] 		; Calculate address of kernel64load module
	shl eax, 4				; Pass it as argument to elfparser
	xor ebx, ebx
	mov bx, [dap_kernel64load + 4]		; which in turn will pass it to the kernel32
	add eax, ebx
	push eax

	xor eax, eax		; Pass address of the DAP of kernel64elf to kernel32
	mov ax, ds		; through elfparse32
	shl eax, 4
	xor ebx, ebx
	mov bx, dap_kernel64elf
	add eax, ebx
	push eax

	xor eax, eax			; Pass the info_table 32-bit address to the kernel32 through elfparse32
	mov ax, [info_table_segment]
	shl eax, 4
	xor ebx, ebx
	mov bx, [info_table_offset]
	add eax, ebx
	push eax

	mov bx, [info_table_segment]		; Load the GDT in GDTR
	mov es, bx				; GDT descriptor is at offset 12 in our custom table
	mov bx, [info_table_offset]
	add bx, 12
	lgdt [es:bx]

	cli				; Disable interrupts
	mov eax, cr0			; Enable protected bit
	or eax, 1			; by setting cr0.bit0 = 1
	mov cr0, eax
	mov ax, 0x18			; Load task register with TSS
	ltr ax
	mov ax, 0x10			; Load the data segment
	mov ds, ax			; and other segments
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
	mov eax, 0xDEADBEEF		; Just a random check
	jmp dword 0x0008:0x00020000	; Jump to kernel32 ELF parser and bye-bye real mode

	mov si, kernel32_jmp_failed		; This should never get executed
	int 22h
	jmp end

diskerror:
	mov si, disk_error
	int 22h
	jmp end

corefileerror:
	mov si, corefiles_error
	int 22h

end:
	mov si, boot_end
	int 22h
	cli							; Disable interrupts
	hlt							; Halt the processor
	jmp end						; Jump to end again just in case if the processor resumes its operation

times 2048 - ($-$$) db 0
