; Black Gate U7.EXE, resident segment 122 (file offsets 0x03da2a to 0x03da4e, 36 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.
; Folder inferred from compiler flags and link order.

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
