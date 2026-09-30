; Black Gate U7.EXE, one module of resident segment 27 (file offsets 0x014d54 to 0x014e1b, 199 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.
; Segment 27 joins 25 modules' code as LOWLEVEL_TEXT, in link order and doubleword aligned.
; This one starts at 0000h; its own empty segment 26 fixes its link position.
; Holds whole-DAC palette loads and reads.

	.MODEL  MEDIUM
	.386
	LOCALS

	PUBLIC  _SetPaletteByIndex, _ReadDacPalette, _WriteDacPalette

	EXTRN   _WaitForRetrace:FAR

LOWLEVEL_TEXT   SEGMENT DWORD PUBLIC USE16 'CODE'
	ASSUME  cs:LOWLEVEL_TEXT

; Set all 256 DAC colors after the vertical retrace, each picked by index
; from a flat table of 6-byte entries.
_SetPaletteByIndex  PROC FAR
	ARG table:DWORD, indices:WORD
	LOCAL   dac:DWORD, colors:BYTE:768 = frame
	enter   frame, 0
	push    edi
	push    esi
	push    ds
	push    es
	push    ds
	pop es
	lea di, colors
	push    ss
	push    di
	pop eax
	mov dac, eax
	mov si, indices
	mov ebx, table
	mov cx, 256
	xor eax, eax
	mov ds, ax
; gather the high bytes of each entry's three words
@@color:
	xor edx, edx
	xor eax, eax
	mov dx, es:[si]
	inc si
	inc si
	mov ax, dx
	shl dx, 1
	add dx, ax
	shl dx, 1       ; index * 6
	push    ebx
	add ebx, edx
	mov ax, [ebx]
	add ebx, 2
	mov al, ah
	stosb
	mov ax, [ebx]
	add ebx, 2
	mov al, ah
	stosb
	mov ax, [ebx]
	add ebx, 2
	mov al, ah
	stosb
	pop ebx
	dec cx
	jne @@color
	pushad
	mov ax, ss
	mov ds, ax
	call    far ptr _WaitForRetrace
	mov ax, 1012h
	xor bx, bx
	mov cx, 256
	mov es, word ptr dac+2
	mov dx, word ptr dac
	int 10h
	popad
	pop es
	pop ds
	pop esi
	pop edi
	leave
	ret
_SetPaletteByIndex  ENDP

; Read all 256 DAC registers into a far buffer.
_ReadDacPalette PROC FAR
	ARG buffer:DWORD
	enter   0, 0
	push    ax
	push    bx
	push    cx
	push    dx
	push    es
	mov ax, 1017h
	xor bx, bx
	mov cx, 256
	mov es, word ptr buffer+2
	mov dx, word ptr buffer
	int 10h
	pop es
	pop dx
	pop cx
	pop bx
	pop ax
	leave
	ret
_ReadDacPalette ENDP

; Set all 256 DAC registers from a far buffer.
_WriteDacPalette    PROC FAR
	ARG buffer:DWORD
	enter   0, 0
	push    ax
	push    bx
	push    cx
	push    dx
	push    es
	mov ax, 1012h
	xor bx, bx
	mov cx, 256
	mov es, word ptr buffer+2
	mov dx, word ptr buffer
	int 10h
	pop es
	pop dx
	pop cx
	pop bx
	pop ax
	leave
	ret
_WriteDacPalette    ENDP

LOWLEVEL_TEXT   ENDS

	END
