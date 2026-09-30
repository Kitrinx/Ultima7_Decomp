; Black Gate INTRO.EXE, resident segment 2 (file offsets 0x00939a to 0x009439, 159 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

	.MODEL  MEDIUM, C

	PUBLIC  DrawStatic

; A view: the video segment, its near table of row offsets, and its clip box,
; corners included.
VIEWREC STRUC
view_seg    dw  ?
view_rows   dw  ?
view_left   dw  ?
view_top    dw  ?
view_right  dw  ?
view_bottom dw  ?
VIEWREC ENDS

BIOS_ROM    EQU 0F000h

	.DATA
Seed        dd  87654321h

	.CODE

; Fills a view with television static: each pixel is either the colour or
; black, picked by the low bit of a byte of the BIOS ROM. A new random start
; in the ROM is taken for each of the frames.
DrawStatic PROC FAR USES si di ds, view:WORD, frames:WORD, color:WORD
	LOCAL   wide:WORD, rows:WORD, left:WORD, topRow:WORD
	mov     bx, view
	mov     es, [bx].view_seg
	mov     cx, [bx].view_left
	mov     left, cx
	mov     dx, [bx].view_right
	inc     dx
	sub     dx, cx
	mov     wide, dx
	mov     dx, [bx].view_top
	mov     cx, [bx].view_bottom
	inc     cx
	sub     cx, dx
	mov     rows, cx
	shl     dx, 1
	mov     topRow, dx
	mov     ax, color
	mov     ah, al
	mov     bx, [bx].view_rows
	cmp     frames, 0
	je      @@done
	mov     cx, BIOS_ROM
	mov     ds, cx                  ; DGROUP is reached through SS from here on
@@frame:
	push    rows
	push    ax
	push    bx
	push    dx
	mov     cx, word ptr ss:Seed    ; Seed = Seed * 10003h
	mov     bx, cx
	mov     ax, word ptr ss:Seed+2
	mov     dx, ax
	add     cx, cx
	adc     ax, ax
	add     cx, bx
	adc     ax, dx
	add     ax, bx
	mov     word ptr ss:Seed, cx
	mov     word ptr ss:Seed+2, ax
	mov     si, ax
	pop     dx
	pop     bx
	pop     ax
@@row:
	mov     di, dx
	mov     di, ss:[bx+di]
	add     di, left
	mov     cx, wide
@@pixel:
	lodsb
	add     si, di                  ; skip ahead by the pixel's address
	test    al, 1
	mov     al, ah
	jnz     @@put
	xor     al, al
@@put:
	stosb
	loop    @@pixel
	add     dx, 2
	dec     rows
	jnz     @@row
	pop     rows
	mov     dx, topRow
	dec     frames
	jnz     @@frame
@@done:
	ret
DrawStatic ENDP

	END
