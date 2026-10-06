; Serpent Isle SI.EXE, resident segment 132 (file offsets 0x03db34 to 0x03db5d, 41 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

	.MODEL  MEDIUM, PASCAL

	PUBLIC  READFILEBLOCK

	EXTRN   DOSREAD:FAR                 ; the block read

	.CODE

; Read count bytes at pos into buf, keeping the registers the read uses.
READFILEBLOCK   PROC FAR handle:WORD, pos:DWORD, count:DWORD, buf:DWORD
	USES    si, di, bx, cx
	push    handle
	push    word ptr pos+2
	push    word ptr pos
	push    word ptr count+2
	push    word ptr count
	push    word ptr buf+2
	push    word ptr buf
	call    DOSREAD
	ret
READFILEBLOCK   ENDP

	END
