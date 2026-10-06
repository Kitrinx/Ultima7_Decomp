; Serpent Isle SI.EXE, resident segment 123 (file offsets 0x03d5ca to 0x03d5fb, 49 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

	.MODEL  MEDIUM, PASCAL

	PUBLIC  FILLFARBYTES

	EXTRN   NORMALIZEPOINTER:FAR

	.CODE

; Fill count bytes at dest with value.
FILLFARBYTES    PROC FAR dest:DWORD, count:WORD, value:WORD
	USES    si, di, es
	pushf
	cld
	mov     ax, word ptr dest+2
	push    ax
	mov     ax, word ptr dest
	push    ax
	call    NORMALIZEPOINTER            ; normalize dest
	mov     di, ax
	mov     es, dx
	mov     cx, count
	mov     ax, value
	mov     ah, al
	shr     cx, 1
	rep     stosw
	rcl     cx, 1
	rep     stosb
	popf
	ret
FILLFARBYTES    ENDP

	END
