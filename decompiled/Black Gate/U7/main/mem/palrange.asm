; Black Gate U7.EXE, one module of resident segment 27 (file offsets 0x01504c to 0x0151ef, 419 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.
; Segment 27 joins 25 modules' code as LOWLEVEL_TEXT, in link order and doubleword aligned.
; This one starts at 02F8h; its own empty segment 146 fixes its link position.
; Holds setting and reading a range of DAC colors.

	.MODEL  MEDIUM
	.386
	LOCALS

DAC_INDEX   EQU 3C8h                    ; DAC write index; the data port follows it

; A short wait between DAC port writes.
DAC_PAUSE       MACRO
	REPT    10
	nop
	ENDM
	ENDM

	PUBLIC  _SetPaletteRange, _SetPaletteRangeBytes, _ReadPaletteRange

LOWLEVEL_TEXT   SEGMENT DWORD PUBLIC USE16 'CODE'
	ASSUME  cs:LOWLEVEL_TEXT

; Set num DAC colors from first on, each picked by index from a flat
; table of 6-byte entries; pauses between port writes.
_SetPaletteRange    PROC FAR
	ARG     table:DWORD, indices:WORD, first:WORD, num:WORD
	enter   0, 0
	push    edi
	push    esi
	push    ds
	push    es
	push    ds
	pop     es
	mov     si, indices
	mov     ebx, table
	mov     cx, num
	mov     ax, first
	mov     di, ax
	shl     ax, 1
	add     si, ax
	xor     eax, eax
	mov     es, ax
; high bytes of the entry's three words
@@color:
	xor     edx, edx
	xor     eax, eax
	mov     dx, [si]
	inc     si
	inc     si
	push    cx
	mov     ax, dx
	shl     dx, 1
	add     dx, ax
	shl     dx, 1
	push    ebx
	add     ebx, edx
	mov     ax, es:[ebx]
	add     ebx, 2
	mov     ch, ah
	mov     ax, es:[ebx]
	add     ebx, 2
	mov     dl, ah
	mov     ax, es:[ebx]
	add     ebx, 2
	mov     dh, ah
	mov     bx, dx
	xor     ax, ax
	mov     dx, DAC_INDEX
	xor     ax, ax
	mov     ax, di
	inc     di
	out     dx, al
	jmp     @@red
; write red, green and blue, pausing before each; a jump to the next line is a short I/O delay
@@red:
	DAC_PAUSE
	inc     dx
	mov     al, ch
	out     dx, al
	jmp     @@green
@@green:
	DAC_PAUSE
	mov     al, bl
	out     dx, al
	jmp     @@blue
@@blue:
	DAC_PAUSE
	mov     al, bh
	out     dx, al
	jmp     @@next
@@next:
	pop     ebx
	pop     cx
	dec     cx
	jne     @@color
	pop     es
	pop     ds
	pop     esi
	pop     edi
	leave
	ret
_SetPaletteRange    ENDP

; Set num DAC colors from first on, each picked by index from a flat
; table of 3-byte entries.
_SetPaletteRangeBytes   PROC FAR
	ARG     table:DWORD, indices:WORD, first:WORD, num:WORD
	enter   0, 0
	push    edi
	push    esi
	push    ds
	push    es
	push    ds
	pop     es
	mov     si, indices
	mov     ebx, table
	mov     cx, num
	mov     ax, first
	mov     di, ax
	shl     ax, 1
	add     si, ax
	xor     eax, eax
	mov     es, ax
@@color:
	xor     edx, edx
	xor     eax, eax
	mov     dx, [si]
	inc     si
	inc     si
	push    cx
	mov     ax, dx
	shl     dx, 1
	add     dx, ax
	push    ebx
	add     ebx, edx
	mov     al, es:[ebx]
	inc     ebx
	mov     ch, al
	mov     al, es:[ebx]
	inc     ebx
	mov     dl, al
	mov     al, es:[ebx]
	inc     ebx
	mov     dh, al
	mov     bx, dx
	xor     ax, ax
	mov     dx, DAC_INDEX
	xor     ax, ax
	mov     ax, di
	inc     di
	out     dx, al
	jmp     @@red
@@red:
	inc     dx
	mov     al, ch
	out     dx, al
	jmp     @@green
@@green:
	mov     al, bl
	out     dx, al
	jmp     @@blue
@@blue:
	mov     al, bh
	out     dx, al
	jmp     @@next
@@next:
	pop     ebx
	pop     cx
	dec     cx
	jne     @@color
	pop     es
	pop     ds
	pop     esi
	pop     edi
	leave
	ret
_SetPaletteRangeBytes   ENDP

; Read num DAC colors from first on into a flat table of 3-byte entries,
; each slot picked by index.
_ReadPaletteRange   PROC FAR
	ARG     table:DWORD, indices:WORD, first:WORD, num:WORD
	enter   0, 0
	push    edi
	push    esi
	push    ds
	push    es
	push    ds
	pop     es
	mov     si, indices
	mov     ebx, table
	mov     cx, num
	mov     ax, first
	mov     di, ax
	shl     ax, 1
	add     si, ax
	xor     eax, eax
	mov     es, ax
@@color:
	xor     edx, edx
	xor     eax, eax
	mov     dx, [si]
	inc     si
	inc     si
	push    cx
	mov     ax, dx
	shl     dx, 1
	add     dx, ax
	push    ebx
	add     ebx, edx
	xor     ax, ax
	mov     dx, DAC_INDEX            ; the write index; a read needs 3C7h
	xor     ax, ax
	mov     ax, di
	inc     di
	out     dx, al
	jmp     @@red
@@red:
	inc     dx
	in      al, dx
	mov     es:[ebx], al
	inc     ebx
	jmp     @@green
@@green:
	mov     al, bl
	in      al, dx
	mov     es:[ebx], al
	inc     ebx
	jmp     @@blue
@@blue:
	mov     al, bh
	in      al, dx
	mov     es:[ebx], al
	jmp     @@next
@@next:
	pop     ebx
	pop     cx
	dec     cx
	jne     @@color
	pop     es
	pop     ds
	pop     esi
	pop     edi
	leave
	ret
_ReadPaletteRange   ENDP

LOWLEVEL_TEXT   ENDS

	END
