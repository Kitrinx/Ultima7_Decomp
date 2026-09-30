; Black Gate INTRO.EXE, resident segment 83 (file offsets 0x01a3a8 to 0x01a3be, 22 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

	.MODEL  MEDIUM, PASCAL

	PUBLIC  GETSHAPEFRAMECOUNT

; A shape starts with its size and a table of frame offsets; the first offset
; also marks where the table ends.
SHAPEHDR STRUC
shape_size          dd  ?
shape_firstFrame    dd  ?
SHAPEHDR ENDS

	.CODE

; Return how many frames a shape holds, from its first frame offset.
GETSHAPEFRAMECOUNT PROC FAR shape:DWORD
	push    si
	push    ds
	lds     si, shape
	mov     ax, word ptr [si].shape_firstFrame
	shr     ax, 1
	shr     ax, 1
	dec     ax
	pop     ds
	pop     si
	ret
GETSHAPEFRAMECOUNT ENDP

	END
