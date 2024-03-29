; HemalOS mayv2

[bits 16]
[org 0x0000]

; Bootloader is loaded from an El-Torito standard "no emulation" boot CD

BOOTLOADER_PADDING equ 32
BOOTLOADER_SEGMENT equ 0x07c0
BOOTLOADER_STACK_SIZE equ 0x0c00
DAP_OFFSET equ 4
DAP_SEGMENT equ 6
ELFPARSE_ORIGIN equ 0x00020000
ISO_SECTOR_SIZE equ 2048

jmp BOOTLOADER_SEGMENT:start	; Ensure cs=0x07c0

times 8 - ($-$$) db 0
printString:
	pusha
	mov ah, 0x0e				; Setup teletype mode
	cld						; Clear direction flag to move towards higher addresses
printStringLoop:
	lodsb					; load character from string at ds:si
	cmp al, 0				; Check if C-style null character
	je printStringEnd		; if null character, end the printString interrupt
	int 0x10				; call int 10 to display character
	jmp printStringLoop	; loop until end of string
printStringEnd:
	popa
	iret					; Return from interrupt

times BOOTLOADER_PADDING - ($-$$) db 0

dapX64:						; Disk address packet (DAP)
	db 0x10						; Size of DAP (always 0x10)
	db 0x00						; Always 0x00
	dw 0x0000					; Number of sectors to read
	dw 0x0000					; offset
	dw 0x0000					; segment
	dq 0x00000000				; starting sector number

dapMmap:
	db 0x10
	db 0x00
	dw 0x0000
	dw 0x0000
	dw 0x0000
	dq 0x00000000

dapVidModes:
	db 0x10
	db 0x00
	dw 0x0000
	dw 0x0000
	dw 0x0000
	dq 0x00000000

dapA20Enable:
	db 0x10
	db 0x00
	dw 0x0000
	dw 0x0000
	dw 0x0000
	dq 0x00000000

dapGdt:
	db 0x10
	db 0x00
	dw 0x0000
	dw 0x0000
	dw 0x0000
	dq 0x00000000

dapKernel64Load:
	db 0x10
	db 0x00
	dw 0x0000
	dw 0x0000
	dw 0x0000
	dq 0x00000000

dapElfParse:
	db 0x10
	db 0x00
	dw 0x0000
	dw 0x0000
	dw 0x0000
	dq 0x00000000

dapLoader32Elf:
	db 0x10
	db 0x00
	dw 0x0000
	dw 0x0000
	dw 0x0000
	dq 0x00000000

dapKernel64Elf:
	db 0x10
	db 0x00
	dw 0x0000
	dw 0x0000
	dw 0x0000
	dq 0x00000000

rootFsGuid:
	times 36 db 0

; Magic bytes
x64Magic db 'x64', 0
mmapMagic db 'MMAP'
vidModesMagic db 'VIDM'
a20EnableMagic db 'A20', 0
gdtMagic db 'GDT', 0
kernel64LoadMagic db 'K64L'
elf32ParseMagic db 'ELFP'
loader32ElfMagic db 'L32E'

driveNumber dw 0
infoTableOffset dw 0
infoTableSegment dw 0x50
helloMsg db 'Welcome to Hemal', 39, 's OS! Booting.....', 13, 10, 13, 10, 0
coreFilesErrorMsg db 'Some core files cannot be found! Cannot boot!', 13, 10, 0
diskErrorMsg db 'Boot disk error! Cannot Boot!', 13, 10, 0
bootEnd db 'Bootloader end', 13, 10, 0
loader32JumpFailed db 'Jumping to kernel failed. Cannot boot!', 13, 10, 0
returnX64 db 'returned form x64.bin', 13, 10, 0

start:
	cli						; Disable interrupts
	mov ax, cs
	mov ds, ax
	mov ax, [infoTableSegment]
	mov es, ax
	mov [driveNumber], dl	; Store the disk number from which this bootloader was loaded
	mov bx, [infoTableOffset]
	xor dh, dh
	mov [es:bx], dx			; Store the disk number from which this bootloader was loaded in InfoTable

	; Setup int 0x22. Each interrupt vector is 4 bytes long.
	; Word[0] is offset and word[1] is code segment.
	xor ax, ax
	mov es, ax
	mov word [es:(0x22 * 4)], printString
	mov word [es:(0x22 * 4 + 2)], cs

	; Set up 3 KiB stack space before bootloader starting from 0x7000 to 0x7c00
	mov ax, (BOOTLOADER_SEGMENT - (BOOTLOADER_STACK_SIZE >> 4))
	mov ss, ax
	mov sp, BOOTLOADER_STACK_SIZE

	mov ax, 0x0003				; Set 80x25 text mode
	int 0x10

	mov si, helloMsg					; Show hello world message on screen

	mov ah, 0x41						; Check disk extensions
	int 0x13
	jc diskError						; No support for disk extensions if carry flag set

	mov cx, 8						; Load 8 core files (subject to change if more files are needed to kick start kernel64)
	mov si, dapX64						; Load address of the first entry
	mov di, x64Magic					; Load address of the first magic number
	mov dl, [driveNumber]					; Boot drive remains consistent for all drive operations

loadCoreFilesLoop:
	mov ah, 0x42						; Extended function 0x42 of int 0x13
	push cx
	mov cx, 3						; Try reading correct data 3 times
retryLoop:
	pusha
	int 0x13							; Load data from disk
	popa
	mov ax, [si + DAP_SEGMENT]					; Get segment
	mov es, ax
	mov bx, [si + DAP_OFFSET]					; Get offset
	mov eax, [es:bx + 8]					; Get magic bytes from loaded data at offset 8
	mov ebx, [di]
	cmp ebx, [loader32ElfMagic]						; Check if it is loader32
	jne loadCoreFilesCont1
	mov bx, [si + DAP_OFFSET]
	mov eax, [es:bx]						; First 4 bytes of an ELF are magic bytes 0x7f, 'ELF'
	mov ebx, 0x464C457F					; Put those magic bytes in ebx in little endian
loadCoreFilesCont1:
	cmp eax, ebx							; Compare magic bytes
	je loadCoreFilesCont
	dec cx
	jnz retryLoop
	jmp diskError
loadCoreFilesCont:
	pop cx
	add si, 16							; Go to next DAP
	add di, 4
	loop loadCoreFilesLoop

	; Optimize BIOS for 64-bit long mode
	call far [dapX64 + DAP_OFFSET]
	mov ax, 0xec00
	mov bl, 2
	int 0x15

	push dword [infoTableOffset]
	call far [dapMmap + DAP_OFFSET]				; Generate memory map

	; push dword [infoTableOffset]
	; call far [dapVidModes + DAP_OFFSET]			; Get list of all video modes
	
	call far [dapA20Enable + DAP_OFFSET]			; Enable the A20 line to access above 1MiB

	push dword [infoTableOffset]
	call far [dapGdt + DAP_OFFSET]				; Setup GDT

	mov sp, BOOTLOADER_STACK_SIZE

	xor eax, eax
	mov ax, [dapKernel64Load + DAP_SEGMENT] 		; Calculate address of kernel64load module
	shl eax, 4				; Pass it as argument to elfparser
	xor ebx, ebx
	mov bx, [dapKernel64Load + DAP_OFFSET]		; which in turn will pass it to the loader32
	add eax, ebx
	push eax

	xor eax, eax		; Pass address of the DAP of kernel64 to loader32
	mov ax, ds			; through elfparse32
	shl eax, 4
	xor ebx, ebx
	mov bx, dapKernel64Elf
	add eax, ebx
	push eax

	xor eax, eax			; Pass the infoTable 32-bit address to the loader32 through elfparse32
	mov ax, [infoTableSegment]
	shl eax, 4
	xor ebx, ebx
	mov bx, [infoTableOffset]
	add eax, ebx
	push eax

	mov bx, [infoTableSegment]		; Load the GDT in GDTR
	mov es, bx				; GDT descriptor is at offset 12 in InfoTable
	mov bx, [infoTableOffset]
	add bx, 12
	lgdt [es:bx]

	; Store the root FS GUID in InfoTable
	add bx, 52
	mov si, rootFsGuid
	mov ecx, 9
rootFsLoop:
	mov edx, [si]
	mov [es:bx], edx
	add si, 4
	add bx, 4
	loop rootFsLoop

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
	; Jump to loader32 ELF parser and bye-bye real mode
	jmp dword 0x0008:ELFPARSE_ORIGIN

	mov si, loader32JumpFailed		; This should never get executed
	int 0x22
	jmp end

diskError:
	mov si, diskErrorMsg
	int 0x22
	jmp end

coreFileError:
	mov si, coreFilesErrorMsg
	int 0x22

end:
	mov si, bootEnd
	int 0x22
	cli							; Disable interrupts
	hlt							; Halt the processor
	jmp end						; Jump to end again just in case if the processor resumes its operation

times ISO_SECTOR_SIZE - ($-$$) db 0
