; Black Gate INTRO.EXE, resident segment 86 (file offsets 0x01a434 to 0x01a45e, 42 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

	.MODEL  MEDIUM, PASCAL
	LOCALS

	PUBLIC  GETEMSSHAPEFRAMECOUNT

	EXTRN   C MapEmsPointer:FAR
	EXTRN   GETSHAPEFRAMECOUNT:FAR

	.CODE

; The frame count of a shape that may live in expanded memory.
GETEMSSHAPEFRAMECOUNT PROC FAR shape:DWORD
	push    word ptr shape+2
	push    word ptr shape
	call    MapEmsPointer
	add     sp, 4
	or      dx, dx
	je      @@done
	mov     word ptr shape, ax
	mov     word ptr shape+2, dx
	push    word ptr shape+2
	push    word ptr shape
	call    GETSHAPEFRAMECOUNT
@@done:
	ret
GETEMSSHAPEFRAMECOUNT ENDP

	END
