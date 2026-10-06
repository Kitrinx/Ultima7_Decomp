; Serpent Isle ENDGAME.EXE, resident segment 85 (file offsets 0x014fe2 to 0x015040, 94 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

.MODEL  MEDIUM, C
	LOCALS

LINE_BYTES      EQU     320

	PUBLIC  DecodeDelta

	.CODE

; Apply a FLIC LC chunk: the first line changed and how many follow, then per
; line a count of packets, each a skip and either bytes to copy or, when the
; count is negative, one byte repeated.
DecodeDelta     PROC FAR data:DWORD, screen:DWORD
	LOCAL   lines:WORD, unused:WORD
	USES    es, ds, si, di, bx, cx
	cld
	lds     si, data
	les     di, screen
	lodsw
	mov     dx, LINE_BYTES
	mul     dx
	add     di, ax
	lodsw
	mov     lines, ax
	mov     dx, di                      ; start of the line
	xor     ah, ah
@@line:
	mov     di, dx
	lodsb
	mov     bl, al                      ; packets in this line
	test    bl, bl
	jmp     @@next
@@packet:
	lodsb
	add     di, ax                      ; bytes left as they are
	lodsb
	test    al, al
	js      @@run
	mov     cx, ax
	rep     movsb
	dec     bl
	jne     @@packet
	jmp     @@lineDone
@@run:
	neg     al
	mov     cx, ax
	lodsb
	rep     stosb
	dec     bl
@@next:
	jne     @@packet
@@lineDone:
	add     dx, LINE_BYTES
	dec     lines
	jne     @@line
	ret
DecodeDelta     ENDP

	END
