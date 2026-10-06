; Serpent Isle INTRO.EXE, resident segment 38 (file offsets 0x00e55a to 0x00e57e, 36 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

	.MODEL  MEDIUM, PASCAL

	PUBLIC  LINEARTOPOINTER

	.CODE

; A far pointer to a linear address, its offset below 16.
LINEARTOPOINTER PROC FAR linear:DWORD
	mov     dx, word ptr linear
	mov     bl, dl
	mov     ax, word ptr linear+2
	shr     al, 1                       ; dx = linear / 16
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
LINEARTOPOINTER ENDP

	END
