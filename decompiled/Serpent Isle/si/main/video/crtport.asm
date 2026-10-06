; Serpent Isle SI.EXE, resident segment 188 (file offsets 0x0404a6 to 0x0404ba, 20 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

	.MODEL  MEDIUM

	PUBLIC  _FindCrtStatusPort, _CrtStatusPort

	.DATA
_CrtStatusPort  dw  0                   ; the CRT status port

	.CODE

; Finds the CRT status port: the controller's base port from the BIOS data area, plus 6.
_FindCrtStatusPort  PROC FAR
	push    es
	mov     ax, 40h
	mov     es, ax
	mov     dx, es:[63h]
	add     dl, 6
	mov     _CrtStatusPort, dx
	pop     es
	ret
_FindCrtStatusPort  ENDP

	END
