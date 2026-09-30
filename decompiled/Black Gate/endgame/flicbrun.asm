; Black Gate ENDGAME.EXE, resident segment 89 (file offsets 0x0158e6 to 0x015935, 79 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

	.MODEL  MEDIUM, C
	LOCALS

LINE_BYTES      EQU     320

	PUBLIC  DecodeBrun

	.CODE

; Unpack a FLIC BRUN chunk: every line is a count of packets, each a run of
; one byte repeated or, when its count is negative, bytes copied as they are.
DecodeBrun      PROC FAR data:DWORD, screen:DWORD, lines:WORD
	LOCAL   unused:DWORD
	USES    es, ds, si, di, bx, cx
	cld
	lds     si, data
	les     di, screen
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
	test    al, al
	js      @@literal
	mov     cx, ax
	lodsb
	rep     stosb
	dec     bl
	jne     @@packet
	jmp     @@lineDone
@@literal:
	neg     al
	mov     cx, ax
	rep     movsb
	dec     bl
@@next:
	jne     @@packet
@@lineDone:
	add     dx, LINE_BYTES
	dec     lines
	jne     @@line
	ret
DecodeBrun      ENDP

	END
