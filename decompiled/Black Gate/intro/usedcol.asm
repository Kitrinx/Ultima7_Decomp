; Black Gate INTRO.EXE, resident segment 71 (file offsets 0x01377c to 0x0137d0, 84 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

	.MODEL  MEDIUM, PASCAL
	LOCALS

	PUBLIC  MARKUSEDCOLORS

; A view: the segment it draws into, a near table of each row's offset in
; that segment, and its clip box, corners included.
VIEWREC STRUC
view_seg    dw  ?
view_rows   dw  ?
view_left   dw  ?
view_top    dw  ?
view_right  dw  ?
view_bottom dw  ?
VIEWREC ENDS

	.CODE

; Set used[c] to 1 for every colour c found in the view's clip box. The table
; is reached through its offset alone, in the data segment.
MARKUSEDCOLORS PROC FAR view:WORD, used:DWORD
	LOCAL   row:WORD, cols:WORD
	USES    si, di
	mov     bx, view
	mov     ax, [bx].view_right
	sub     ax, [bx].view_left
	inc     ax
	mov     cols, ax
	mov     ax, [bx].view_bottom
	sub     ax, [bx].view_top
	mov     row, ax
	mov     es, [bx].view_seg
	cld
	mov     di, word ptr used
	mov     ah, 1
	mov     dx, bx
; bottom row first
@@row:
	mov     si, [bx].view_top
	add     si, row
	shl     si, 1
	add     si, [bx].view_rows
	mov     si, [si]
	add     si, [bx].view_left
	mov     cx, cols
	xor     bh, bh
@@pixel:
	lods    byte ptr es:[si]
	mov     bl, al
	mov     [bx+di], ah
	loop    @@pixel
	mov     bx, dx
	dec     row
	jns     @@row
	ret
MARKUSEDCOLORS ENDP

	END
