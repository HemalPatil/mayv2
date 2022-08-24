[bits 16]
[org 0x0000]
jmp start

INFOTABLE_MMAP_ENTRIES_OFFSET equ 4
INFOTABLE_MMAP_ENTRIES_SEGMENT equ 6
L32K64_SCRATCH_BASE equ 0x80000
K64_SCRATCH_LENGH equ 0x10000
MMAP_ENTRIES_OFFSET equ 0
MMAP_ENTRIES_SEGMENT equ 0x0060

times 8 - ($-$$) db 0

magicBytes db 'MMAP'
mmapMsg db 'Generating Memory Map', 13, 10, 0
mmapDoneMsg db 'Memory Map complete', 13, 10, 13, 10, 0
mmapFailMsg db 'Memory Map failed. ', 0
cannotBootMsg db 'Cannot boot! ', 13, 10, 0
insufficientMemoryMsg db 'Insufficient memory detetcted. ', 0
num1Low dd 0
num1High dd 0
num2Low dd 0
num2High dd 0
valid dw 0

start:
	push bp
	mov bp, sp
	pusha
	push ds
	mov ax, cs
	mov ds, ax

	mov si, mmapMsg					; Generating memory map entries
	int 0x22

	; Store address of MMAP entries in InfoTable
	mov ax, [bp + 8]
	mov es, ax
	mov bx, [bp + 6]
	mov word [es:bx + INFOTABLE_MMAP_ENTRIES_OFFSET], MMAP_ENTRIES_OFFSET
	mov word [es:bx + INFOTABLE_MMAP_ENTRIES_SEGMENT], MMAP_ENTRIES_SEGMENT

	push bp

	; Set up the memory map with int 0x15
	; Courtesy of https://wiki.osdev.org/Detecting_Memory_(x86)
mmap:
	mov bx, MMAP_ENTRIES_SEGMENT
	mov es, bx
	xor ebx, ebx					; ebx must be 0 to start
	xor bp, bp						; keep an entry count in bp
	mov edx, 0x0534D4150			; Place "SMAP" into edx
	mov eax, 0xe820
	xor di, di
	mov [es:di + 20], dword 1		; force a valid ACPI 3.X entry
	mov ecx, 24						; ask for 24 bytes
	int 0x15
	jc mmapFail				; carry set on first call means "unsupported function"
	mov edx, 0x0534D4150			; Some BIOSes apparently trash this register?
	cmp eax, edx					; on success, eax must have been reset to "SMAP"
	jne mmapFail
	test ebx, ebx					; ebx = 0 implies list is only 1 entry long (worthless)
	je mmapFail
	jmp mmapInside
mmapLoop:
	mov eax, 0xe820					; eax, ecx get trashed on every int 0x15 call
	mov [es:di + 20], dword 1		; force a valid ACPI 3.X entry
	mov ecx, 24						; ask for 24 bytes again
	int 0x15
	jc mmapExit				; carry set means "end of list already reached"
	mov edx, 0x0534D4150			; repair potentially trashed register
mmapInside:
	cmp cx, 0					; skip any 0 length entries
	je mmapSkip
	cmp cl, 20						; got a 24 byte ACPI 3.X response?
	jbe short mmapShort
	test byte [es:di + 20], 1		; if so: is the "ignore this data" bit clear?
	je short mmapSkip
mmapShort:
	mov ecx, [es:di + 8]			; get lower uint32_t of memory region length
	or ecx, [es:di + 12]			; "or" it with upper uint32_t to test for zero
	jz mmapSkip					; if length uint64_t is 0, skip entry
	cmp word [valid], 0
	jne mmapValidEntry
	cmp byte [es:di + 16], 1		; Check if memory region type is 1, i.e. free for use
	jne mmapValidEntry		; If not skip this validation of this entry
	push eax
	mov eax, [es:di]				; Check if the memory 0x80000 to 0x90000
	mov [num1Low], eax			; is available in the same memory region
	mov eax, [es:di + 4]			; and the memory is usable
	mov [num1High], eax
	xor eax, eax
	mov [num2High], eax
	mov dword [num2Low], L32K64_SCRATCH_BASE
	pop eax
	call cmpUint64
	ja mmapValidEntry
	push eax
	mov eax, [es:di + 8]
	mov [num2Low], eax
	mov eax, [es:di + 12]
	mov [num2High], eax
	call addUint64
	xor eax, eax
	mov [num2High], eax
	mov eax, L32K64_SCRATCH_BASE + K64_SCRATCH_LENGH
	mov [num2Low], eax
	pop eax
	call cmpUint64
	jb mmapValidEntry
	mov word [valid], 1
mmapValidEntry:
	inc bp							; got a good entry: + + count, move to next storage spot
	add di, 24
mmapSkip:
	test ebx, ebx					; if ebx resets to 0, list is complete
	jne mmapLoop
mmapExit:
	mov cx, bp
	pop bp
	mov bx, [bp + 8]
	mov es, bx
	mov bx, [bp + 6]
	mov [es:bx + 2], cx					; store the entry count in InfoTable
	mov si, mmapDoneMsg			; Show mmap complete message
	int 0x22
	clc								; there is "jc" on end of list to this point, so the carry must be cleared
	jmp mmapFinish
mmapFail:
	mov si, mmapFailMsg			; Show mmap fail message
	int 0x22
	mov si, cannotBootMsg
	int 0x22
	stc								; "function unsupported" error exit
errorEnd:
	cli
	hlt
	jmp errorEnd
mmapFinish:
	cmp word [valid], 1
	jne insufficientMemory

	; Check memory size (atleast 1GiB of free usable conventional memory is required)
	mov bx, [bp + 8]					; Custom table stored at 0x0050:0x0000
	mov es, bx
	mov bx, [bp + 6]
	xor eax, eax
	xor ecx, ecx
	mov cx, [es:bx + 2]				; Get count of MMAP entries
	mov di, [es:bx + 4]				; Get offset and segment of MMAP entries from InfoTable
	mov ax, [es:bx + 6]
	mov es, ax
	xor eax, eax			; Store length in eax
	mov edx, 0x40000000		; Put 1GiB in edx
memCountLoop:
	cmp byte [es:di + 16], 1		; Check memory type (should be 1 for conventional memory)
	jne memCountSkip		; If not 1 then skip this entry
	mov ebx, [es:di + 12]		; Check higher 32 bits of uint64_t length
	test ebx, ebx			; If not zero then this entry is greater than 1 GiB
	jnz greaterThan1GiB
	mov ebx, [es:di + 8]		; Check if lower 32 bits of uint64_t length
	cmp ebx, edx			; is greater than 1GiB
	jnb greaterThan1GiB
	add eax, ebx			; Add the length of this region to mem count so far
	jc greaterThan1GiB		; If overflows, then greater than 1GiB
memCountSkip:
	add di, 24		; Move to next entry
	loop memCountLoop

insufficientMemory:
	mov si, insufficientMemoryMsg				; Insufficient memory found in the system
	int 0x22							; Halt processor completely
	mov si, cannotBootMsg
	int 0x22
insufficientMemoryErrorEnd:
	cli
	hlt
	jmp insufficientMemoryErrorEnd

greaterThan1GiB:
	pop ds
	popa
	mov sp, bp
	pop bp
	retf 4

addUint64:
	push eax		; Add lower 32 bits and then add higher 32 bits along with carry
	mov eax, [num1Low]
	add eax, [num2Low]
	mov [num1Low], eax
	mov eax, [num1High]
	adc eax, [num2High]
	mov [num1High], eax
	pop eax
	ret

cmpUint64:
	push eax		; Compare higher 32 bits
	mov eax, [num1High]	; if they are same, compare lower 32 bits
	cmp eax, [num2High]
	jne cmpEnd
	mov eax, [num1Low]
	cmp eax, [num2Low]
cmpEnd:
	pop eax
	ret
