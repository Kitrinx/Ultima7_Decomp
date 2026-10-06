; Serpent Isle SI.EXE, resident segment 120 (file offsets 0x03d54c to 0x03d584, 56 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

	.MODEL  MEDIUM, PASCAL

	PUBLIC  NORMALIZEPOINTER

	.CODE

; p with its offset brought below 16.
NORMALIZEPOINTER    PROC FAR p:DWORD
	mov     dx, word ptr p+2
	xor     ax, ax
	shl     dx, 1                       ; al:dx = segment * 16
	rcl     al, 1
	shl     dx, 1
	rcl     al, 1
	shl     dx, 1
	rcl     al, 1
	shl     dx, 1
	rcl     al, 1
	add     dx, word ptr p
	adc     al, ah
	mov     bl, dl
	shr     al, 1                       ; dx = the address / 16
	rcr     dx, 1
	shr     al, 1
	rcr     dx, 1
	shr     al, 1
	rcr     dx, 1
	shr     al, 1
	rcr     dx, 1
	mov     al, bl
	and     ax, 0Fh
	ret
NORMALIZEPOINTER    ENDP

	END
