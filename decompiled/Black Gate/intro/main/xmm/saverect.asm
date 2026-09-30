; Black Gate INTRO.EXE, resident segment 72 (file offsets 0x0137d0 to 0x0138af, 223 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

	.MODEL  MEDIUM, C
	LOCALS

	PUBLIC  SaveRect

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

; Copy the part of a rectangle that lies on a view into a block the
; rectangle's size, so RestoreRect can put it back.
SaveRect    PROC FAR view:WORD, buffer:DWORD, rect:WORD
	LOCAL   boxTop:WORD, boxLeft:WORD, boxWidth:WORD, boxHeight:WORD, clipTop:WORD, \
		clipLeft:WORD, cols:WORD, rows:WORD, rowSeg:WORD
	USES    si, di, ds
; rectangle size, then clip its left edge
	push    cx
	mov     si, rect
	mov     bx, view
	mov     cx, [si].rect_left
	mov     ax, [si].rect_right
	sub     ax, cx
	inc     ax
	mov     boxWidth, ax
	mov     cols, ax
	mov     ax, [si].rect_bottom
	mov     cx, [si].rect_top
	sub     ax, cx
	inc     ax
	mov     boxHeight, ax
	mov     rows, ax
	pop     cx
	mov     ax, [si].rect_left
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
	mov     ax, [si].rect_top
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
	mov     ax, [si].rect_right
	cmp     ax, [bx].view_left
	jl      @@offView
	sub     ax, [bx].view_right
	jle     @@rightDone
	sub     cols, ax
@@rightDone:
	mov     ax, [si].rect_bottom
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
	mov     di, ax
	mov     dx, clipLeft
	sub     dx, boxLeft
	add     di, dx
	mov     ax, [bx].view_seg
	mov     rowSeg, ax
	mov     es, word ptr buffer+2
	add     di, word ptr buffer
	mov     ax, clipTop
	shl     ax, 1
	add     ax, [bx].view_rows
	mov     bx, ax
	mov     ds, rowSeg
; copy one row of the view into the block
@@row:
	mov     si, ss:[bx]
	add     si, clipLeft
	mov     cx, cols
	shr     cx, 1
	rep     movsw
	rcl     cx, 1
	rep     movsb
	add     bx, 2
	sub     di, cols
	add     di, boxWidth
	dec     rows
	jne     @@row
@@done:
	ret
SaveRect    ENDP

	END
