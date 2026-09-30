; Black Gate U7.EXE, resident segment 20 (file offsets 0x0128fc to 0x012912, 22 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.
; Folder chosen by subsystem.

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
