; Serpent Isle MAINMENU.EXE, one module of resident segment 19 (file offsets 0x00f7b2 to 0x00f80f, 93 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

	.MODEL  MEDIUM, C
	LOCALS

DAC_INDEX       EQU     3C8h            ; DAC write index; the data port follows it
RETRACE         EQU     08h             ; vertical retrace bit of the CRT status port

	PUBLIC  SetDacColors
	EXTRN   CrtStatusPort:WORD

	.CODE

; Load count colors of palette, three bytes each, into the DAC from first on,
; waiting for vertical retrace before each one.
SetDacColors        PROC FAR palette:WORD, first:WORD, count:WORD
	push    di
	push    si
	push    ds
	push    es
	mov     bx, palette
	mov     cx, count
	mov     ax, first
	mov     di, ax
	push    ds
	pop     es
	mov     dx, ax
	shl     ax, 1
	add     ax, dx
	add     bx, ax                      ; palette + first * 3
@@color:
	xor     dx, dx
	xor     ax, ax
	push    cx
	mov     al, es:[bx]
	inc     bx
	mov     ch, al
	mov     al, es:[bx]
	inc     bx
	mov     dl, al
	mov     al, es:[bx]
	inc     bx
	mov     dh, al
	push    bx
	mov     bx, dx                      ; green and blue
	xor     ax, ax
	mov     dx, CrtStatusPort
@@wait:
	in      al, dx
	test    al, RETRACE
	je      @@wait
	mov     dx, DAC_INDEX
	mov     ax, di
	inc     di
	out     dx, al
	inc     dx
	mov     al, ch
	out     dx, al
	mov     al, bl
	out     dx, al
	mov     al, bh
	out     dx, al
	pop     bx
	pop     cx
	dec     cx
	jne     @@color
	pop     es
	pop     ds
	pop     si
	pop     di
	ret
SetDacColors        ENDP

	END
