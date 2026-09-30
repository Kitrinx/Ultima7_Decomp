; Black Gate U7.EXE, resident segment 190 (file offsets 0x040840 to 0x04084f, 15 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.
; Folder inferred from compiler flags and link order.

	.MODEL  MEDIUM
	LOCALS

	PUBLIC  _WaitForRetrace
	EXTRN   _CrtStatusPort:WORD

	.CODE

; Waits for the start of the next vertical retrace.
_WaitForRetrace PROC FAR
	mov     dx, _CrtStatusPort
@@inRetrace:
	in      al, dx
	test    al, 8                       ; vertical retrace bit
	jnz     @@inRetrace
@@notYet:
	in      al, dx
	test    al, 8
	jz      @@notYet
	ret
_WaitForRetrace ENDP

	END
