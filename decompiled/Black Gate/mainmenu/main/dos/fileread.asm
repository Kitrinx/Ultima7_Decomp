; Black Gate MAINMENU.EXE, resident segment 89 (file offsets 0x019ee8 to 0x019f1b, 51 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

	.MODEL  MEDIUM, PASCAL

	PUBLIC  READFILEBLOCK

	EXTRN   C MapEmsPointer:FAR   ; buffer address to a real pointer
	EXTRN   DOSREAD:FAR           ; the block read

	.CODE

; Read into buf count bytes at pos, keeping the registers the read uses.
READFILEBLOCK   PROC FAR handle:WORD, pos:DWORD, count:DWORD, buf:DWORD
	USES    si, di, bx, cx
	push    word ptr buf+2
	push    word ptr buf
	call    MapEmsPointer
	add     sp, 4
	push    handle
	push    word ptr pos+2
	push    word ptr pos
	push    word ptr count+2
	push    word ptr count
	push    dx
	push    ax
	call    DOSREAD
	ret
READFILEBLOCK   ENDP

	END
