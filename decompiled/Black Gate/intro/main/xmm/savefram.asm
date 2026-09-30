; Black Gate INTRO.EXE, resident segment 64 (file offsets 0x0133d2 to 0x01350a, 312 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

	.MODEL  MEDIUM, C
	LOCALS

	PUBLIC  SaveUnderFrame

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

; Copy the part of a view that a shape frame at x, y would cover into a
; block the frame's size, so RestoreUnderFrame can put it back.
SaveUnderFrame PROC FAR view:WORD, buffer:DWORD, x:WORD, y:WORD, shape:DWORD, frameNum:WORD
	LOCAL   boxTop:WORD, boxLeft:WORD, boxWidth:WORD, boxHeight:WORD, clipTop:WORD, \
		clipLeft:WORD, cols:WORD, rows:WORD, rowSeg:WORD
	USES    si, di, ds
	les     si, shape
	mov     bx, frameNum
	inc     bx
	shl     bx, 1
	shl     bx, 1
	cmp     word ptr es:[si].shape_firstFrame, bx
	jb      @@noFrame
	jne     @@haveFrame
@@noFrame:
	jmp     @@done
; point es:si at the frame, normalised so its offset stays under 16
@@haveFrame:
	mov     ax, es
	mov     dx, 0
	REPT    4
	shl     ax, 1
	rcl     dx, 1
	ENDM
	add     ax, si
	adc     dx, 0
	add     ax, es:[bx+si]
	adc     dx, es:[bx+si+2]
	mov     si, ax
	and     si, 0Fh
	REPT    4
	shr     dx, 1
	rcr     ax, 1
	ENDM
	mov     es, ax
; box the frame covers, then clip its left edge
	mov     bx, view
	mov     ax, es:[si].frame_right
	stc
	adc     ax, es:[si].frame_left
	mov     boxWidth, ax
	mov     cols, ax
	mov     ax, es:[si].frame_bottom
	stc
	adc     ax, es:[si].frame_top
	mov     boxHeight, ax
	mov     rows, ax
	mov     ax, x
	sub     ax, es:[si].frame_left
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
	sub     ax, es:[si].frame_top
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
	add     ax, es:[si].frame_right
	cmp     ax, [bx].view_left
	jl      @@offView
	sub     ax, [bx].view_right
	jle     @@rightDone
	sub     cols, ax
@@rightDone:
	mov     ax, y
	add     ax, es:[si].frame_bottom
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
SaveUnderFrame ENDP

	END
