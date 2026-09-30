; Black Gate INTRO.EXE, resident segment 65 (file offsets 0x01350a to 0x013541, 55 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

	.MODEL  MEDIUM, C
	LOCALS

	PUBLIC  DrawEmsFrame

	EXTRN   MapEmsPointer:FAR, DrawFrame:FAR

	.CODE

; DrawFrame for a shape that may sit in expanded memory: map it in first.
DrawEmsFrame    PROC FAR view:WORD, x:WORD, y:WORD, shape:DWORD, frameNum:WORD
	push    word ptr shape+2
	push    word ptr shape
	call    MapEmsPointer
	add     sp, 4
	or      dx, dx
	je      @@done
	mov     word ptr shape, ax
	mov     word ptr shape+2, dx
	push    frameNum
	push    word ptr shape+2
	push    word ptr shape
	push    y
	push    x
	push    view
	call    DrawFrame
	add     sp, 12
@@done:
	ret
DrawEmsFrame    ENDP

	END
