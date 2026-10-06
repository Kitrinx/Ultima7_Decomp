; Serpent Isle SI.EXE, resident segment 133 (file offsets 0x03db5e to 0x03db87, 41 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

	.MODEL  MEDIUM, PASCAL

	PUBLIC  WRITEFILEBLOCK

	EXTRN   DOSWRITE:FAR                ; the block write

	.CODE

; Write count bytes from buf at pos, keeping the registers the write uses.
WRITEFILEBLOCK  PROC FAR handle:WORD, pos:DWORD, count:DWORD, buf:DWORD
	USES    si, di, bx, cx
	push    handle
	push    word ptr pos+2
	push    word ptr pos
	push    word ptr count+2
	push    word ptr count
	push    word ptr buf+2
	push    word ptr buf
	call    DOSWRITE
	ret
WRITEFILEBLOCK  ENDP

	END
