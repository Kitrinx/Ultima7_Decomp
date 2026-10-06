; Serpent Isle MAINMENU.EXE, resident segment 35 (file offsets 0x0122b4 to 0x0122ca, 22 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

	.MODEL  MEDIUM, C

	PUBLIC  GetRomFont

	.CODE

; Far pointer to a ROM character set, from video BIOS function 1130h:
; which is 2 for the 8x14 font, 3 for 8x8, 6 for 8x16.
GetRomFont  PROC FAR which:BYTE
	push    es
	push    bp
	mov     ah, 11h
	mov     al, 30h
	mov     bh, which
	int     10h
	mov     dx, es
	mov     ax, bp
	pop     bp
	pop     es
	ret
GetRomFont  ENDP

	END
