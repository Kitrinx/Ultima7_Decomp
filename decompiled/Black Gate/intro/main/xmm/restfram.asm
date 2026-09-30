; Black Gate INTRO.EXE, resident segment 63 (file offsets 0x01329a to 0x0133d2, 312 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

	.MODEL  MEDIUM, C
	LOCALS

	PUBLIC  RestoreUnderFrame

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

; A shape starts with its size and a table of frame offsets; the first offset
; also marks where the table ends.
SHAPEHDR STRUC
shape_size          dd  ?
shape_firstFrame    dd  ?
SHAPEHDR ENDS

; A frame starts with its extents from the hot spot, then its spans.
FRAMEHDR STRUC
frame_right     dw  ?
frame_left      dw  ?
frame_top       dw  ?
frame_bottom    dw  ?
FRAMEHDR ENDS

	.CODE

; Copy the block saved from under a shape frame at x, y back onto a view,
; clipped to the view.
RestoreUnderFrame PROC FAR view:WORD, buffer:DWORD, x:WORD, y:WORD, shape:DWORD, frameNum:WORD
	LOCAL   boxTop:WORD, boxLeft:WORD, boxWidth:WORD, boxHeight:WORD, clipTop:WORD, \
		clipLeft:WORD, cols:WORD, rows:WORD, bufSeg:WORD
	USES    si, di, ds
	les     di, shape
	mov     bx, frameNum
	inc     bx
	shl     bx, 1
	shl     bx, 1
	cmp     word ptr es:[di].shape_firstFrame, bx
	jb      @@noFrame
	jne     @@haveFrame
@@noFrame:
	jmp     @@done
; point es:di at the frame, normalised so its offset stays under 16
@@haveFrame:
	mov     ax, es
	mov     dx, 0
	REPT    4
	shl     ax, 1
	rcl     dx, 1
	ENDM
	add     ax, di
	adc     dx, 0
	add     ax, es:[bx+di]
	adc     dx, es:[bx+di+2]
	mov     di, ax
	and     di, 0Fh
	REPT    4
	shr     dx, 1
	rcr     ax, 1
	ENDM
	mov     es, ax
; box the frame covers, then clip its left edge
	mov     bx, view
	mov     ax, es:[di].frame_right
	stc
	adc     ax, es:[di].frame_left
	mov     boxWidth, ax
	mov     cols, ax
	mov     ax, es:[di].frame_bottom
	stc
	adc     ax, es:[di].frame_top
	mov     boxHeight, ax
	mov     rows, ax
	mov     ax, x
	sub     ax, es:[di].frame_left
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
	mov     ax, y
	sub     ax, es:[di].frame_top
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
	mov     ax, x
	add     ax, es:[di].frame_right
	cmp     ax, [bx].view_left
	jl      @@offView
	sub     ax, [bx].view_right
	jle     @@rightDone
	sub     cols, ax
@@rightDone:
	mov     ax, y
	add     ax, es:[di].frame_bottom
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
RestoreUnderFrame ENDP

	END
