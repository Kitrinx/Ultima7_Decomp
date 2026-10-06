; Serpent Isle INTRO.EXE, resident segment 87 (file offsets 0x015cdc to 0x015d14, 56 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

	.MODEL  MEDIUM, C
	LOCALS

	PUBLIC  DecodeColor

	.CODE

; Apply a FLIC COLOR chunk to a palette of three-byte colors: a count of
; packets, each a number of colors to skip and a number to copy (0 for 256).
DecodeColor     PROC FAR data:DWORD, palette:DWORD
	USES    ds, si, di, cx, bx
	cld
	lds     si, data
	les     di, palette
	lodsw
	mov     bx, ax                      ; packets
	test    bx, bx
	jmp     @@next
@@packet:
	lodsb
	add     di, ax
	add     di, ax
	add     di, ax
	lodsb
	or      al, al
	jne     @@copy
	mov     ax, 256
@@copy:
	mov     cx, ax
	add     cx, ax
	add     cx, ax
	rep     movsb
	dec     bx
@@next:
	jne     @@packet
	ret
DecodeColor     ENDP

	END
