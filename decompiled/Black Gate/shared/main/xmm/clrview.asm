; Black Gate ENDGAME.EXE, resident segment 69 (file offsets 0x011d30 to 0x011d7d, 77 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

	.MODEL  MEDIUM, C
	LOCALS

	PUBLIC  FillView

; A view: the segment its pixels sit in, its table of row offsets, and its
; clip box, corners included.
VIEWREC STRUC
view_seg    dw  ?
view_rows   dw  ?
view_left   dw  ?
view_top    dw  ?
view_right  dw  ?
view_bottom dw  ?
VIEWREC ENDS

	.CODE

; Fill a view's whole clip box with one color.
FillView       PROC FAR view:WORD, color:WORD
	LOCAL   left:WORD
	USES    si, di
	cld
	mov     ax, color
	mov     ah, al
	mov     bx, view
	mov     es, [bx].view_seg
	mov     dx, [bx].view_left
	mov     left, dx
	mov     dx, [bx].view_right
	sub     dx, left
	inc     dx                          ; width
	mov     si, [bx].view_bottom
	sub     si, [bx].view_top
	inc     si                          ; rows
	mov     di, [bx].view_rows
	mov     bx, [bx].view_top
	shl     bx, 1
	add     bx, di
@@row:
	mov     di, ss:[bx]
	add     di, left
	mov     cx, dx
	shr     cx, 1                       ; words, then the odd byte
	rep     stosw
	rcl     cx, 1
	rep     stosb
	add     bx, 2
	dec     si
	jne     @@row
	ret
FillView       ENDP

	END
