; Black Gate INTRO.EXE, resident segment 67 (file offsets 0x013580 to 0x013666, 230 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

	.MODEL  MEDIUM, C
	LOCALS

	PUBLIC  RestoreRect

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

; A rectangle, corners included.
RECTREC STRUC
rect_left   dw  ?
rect_top    dw  ?
rect_right  dw  ?
rect_bottom dw  ?
RECTREC ENDS

	.CODE

; Copy a block saved from a rectangle back onto a view, clipped to the view.
RestoreRect PROC FAR view:WORD, buffer:DWORD, rect:WORD
	LOCAL   boxTop:WORD, boxLeft:WORD, boxWidth:WORD, boxHeight:WORD, clipTop:WORD, \
		clipLeft:WORD, cols:WORD, rows:WORD, bufSeg:WORD
	USES    si, di, ds
	mov     ax, ds
	mov     es, ax
	mov     di, rect
	mov     bx, view
; rectangle size, then clip its left edge
	push    cx
	mov     ax, [di].rect_right
	mov     cx, [di].rect_left
	sub     ax, cx
	inc     ax
	mov     boxWidth, ax
	mov     cols, ax
	mov     ax, [di].rect_bottom
	mov     cx, [di].rect_top
	sub     ax, cx
	inc     ax
	mov     boxHeight, ax
	mov     rows, ax
	pop     cx
	mov     ax, [di].rect_left
	cmp     ax, [bx].view_right
	jg      @@outside
	mov     boxLeft, ax
	mov     clipLeft, ax
	mov     ax, [bx].view_left
	sub     ax, clipLeft
	jl      @@leftDone
	sub     cols, ax
	mov     ax, [bx].view_left
	mov     clipLeft, ax
; clip the top edge
@@leftDone:
	mov     ax, [di].rect_top
	cmp     ax, [bx].view_bottom
	jle     @@topIn
@@outside:
	jmp     @@done
@@topIn:
	mov     boxTop, ax
	mov     clipTop, ax
	mov     ax, [bx].view_top
	sub     ax, clipTop
	jl      @@topDone
	sub     rows, ax
	mov     ax, [bx].view_top
	mov     clipTop, ax
; clip the right and bottom edges
@@topDone:
	mov     ax, [di].rect_right
	cmp     ax, [bx].view_left
	jl      @@offView
	sub     ax, [bx].view_right
	jle     @@rightDone
	sub     cols, ax
@@rightDone:
	mov     ax, [di].rect_bottom
	cmp     ax, [bx].view_top
@@offView:
	jl      @@done
	sub     ax, [bx].view_bottom
	jle     @@bottomDone
	sub     rows, ax
; start at the visible part of the saved block
@@bottomDone:
	mov     ax, clipTop
	sub     ax, boxTop
	mul     word ptr boxWidth
	mov     si, ax
	mov     dx, clipLeft
	sub     dx, boxLeft
	add     si, dx
	mov     ax, word ptr buffer+2
	mov     bufSeg, ax
	mov     bx, view
	mov     es, [bx].view_seg
	add     si, word ptr buffer
	mov     ax, clipTop
	shl     ax, 1
	add     ax, [bx].view_rows
	mov     bx, ax
	mov     ds, bufSeg
; copy one row back onto the view
@@row:
	mov     di, ss:[bx]
	add     di, clipLeft
	mov     cx, cols
	shr     cx, 1
	rep     movsw
	rcl     cx, 1
	rep     movsb
	add     bx, 2
	sub     si, cols
	add     si, boxWidth
	dec     rows
	jne     @@row
@@done:
	ret
RestoreRect ENDP

	END
