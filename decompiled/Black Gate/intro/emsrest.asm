; Black Gate INTRO.EXE, resident segment 66 (file offsets 0x013542 to 0x01357f, 61 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

	.MODEL  MEDIUM, C
	LOCALS

	PUBLIC  RestoreUnderEmsFrame
	EXTRN   MapEmsPointer:FAR, RestoreUnderFrame:FAR

	.CODE

; Put back what lay under a frame of a shape that may live in expanded memory.
RestoreUnderEmsFrame PROC FAR view:WORD, buffer:DWORD, x:WORD, y:WORD, shape:DWORD, frameNum:WORD
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
	push    word ptr buffer+2
	push    word ptr buffer
	push    view
	call    RestoreUnderFrame
	add     sp, 16
@@done:
	ret
RestoreUnderEmsFrame ENDP

	END
