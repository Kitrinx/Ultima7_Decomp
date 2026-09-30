; Black Gate ENDGAME.EXE, resident segment 72 (file offsets 0x01211e to 0x012262, 324 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

	.MODEL  MEDIUM, C
	LOCALS

	PUBLIC  OutlineRect

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

	.DATA

lineColor   dw  0                       ; the color in both bytes
unused      dw  3 dup (0)
left        dw  0                       ; the edges' ends, clipped
right       dw  0
topRow      dw  0                       ; clip rows, as row table offsets
bottomRow   dw  0
clipLeft    dw  0
clipRight   dw  0
leftX       dw  0                       ; the side columns, unclipped
rightX      dw  0
topY        dw  0                       ; the top and bottom edges' rows, as row table offsets
bottomY     dw  0

	.CODE

; Draw the outline of the rectangle from x0, y0 to x1, y1 in one color,
; clipped to the view. The row table is read through bp, so the arguments
; are copied out first.
OutlineRect     PROC FAR view:WORD, x0:WORD, y0:WORD, x1:WORD, y1:WORD, color:WORD
	USES    si, di
	mov     bx, view
	mov     es, [bx].view_seg
	mov     ax, color
	mov     ah, al
	mov     lineColor, ax
	mov     ax, [bx].view_left
	mov     clipLeft, ax
	mov     ax, [bx].view_right
	mov     clipRight, ax
	mov     si, x0
	cmp     si, clipRight
	jg      @@clipped
	mov     leftX, si
	cmp     si, clipLeft
	jge     @@clipX1
	mov     si, clipLeft
@@clipX1:
	mov     left, si
	mov     si, x1
	cmp     si, clipLeft
	jl      @@clipped
	mov     rightX, si
	cmp     si, clipRight
	jle     @@clipY
	mov     si, clipRight
@@clipY:
	mov     right, si
	mov     ax, [bx].view_top
	shl     ax, 1
	mov     topRow, ax
	mov     ax, [bx].view_bottom
	shl     ax, 1
	mov     bottomRow, ax
	mov     ax, y0
	shl     ax, 1
	cmp     ax, bottomRow
	jg      @@clipped
	mov     topY, ax
	mov     ax, y1
	shl     ax, 1
	cmp     ax, topRow
	jl      @@clipped
	mov     bottomY, ax
	mov     bp, [bx].view_rows
	jmp     short @@top
@@clipped:
	jmp     @@done
; the top edge, unless it is above the view; the sides then start below it
@@top:
	mov     si, topY
	cmp     si, topRow
	jl      @@topClipped
	mov     di, left
	mov     cx, right
	sub     cx, di
	inc     cx
	add     di, [bp+si]
	mov     ax, lineColor
	mov     ah, al
	shr     cx, 1                       ; words, then the odd byte
	rep     stosw
	rcl     cx, 1
	rep     stosb
	add     topY, 2
	jmp     short @@bottom
@@topClipped:
	mov     si, topRow
	mov     topY, si
; the bottom edge, unless it is below the view
@@bottom:
	mov     si, bottomY
	cmp     si, bottomRow
	jg      @@bottomClipped
	mov     di, left
	mov     cx, right
	sub     cx, di
	inc     cx
	add     di, [bp+si]
	mov     ax, lineColor
	mov     ah, al
	shr     cx, 1
	rep     stosw
	rcl     cx, 1
	rep     stosb
	sub     bottomY, 2
	jmp     short @@leftSide
@@bottomClipped:
	mov     si, bottomRow
	mov     bottomY, si
@@leftSide:
	mov     di, leftX
	cmp     di, clipLeft
	jl      @@rightSide
	mov     si, topY
	cmp     si, bottomY
	jg      @@done
	mov     cx, di
	mov     ax, lineColor
@@leftPixel:
	mov     di, cx
	add     di, [bp+si]
	mov     es:[di], al
	add     si, 2
	cmp     si, bottomY
	jle     @@leftPixel
@@rightSide:
	mov     di, rightX
	cmp     di, clipRight
	jg      @@done
	mov     si, topY
	cmp     si, bottomY
	jg      @@done
	mov     cx, di
	mov     ax, lineColor
@@rightPixel:
	mov     di, cx
	add     di, [bp+si]
	mov     es:[di], al
	add     si, 2
	cmp     si, bottomY
	jle     @@rightPixel
@@done:
	ret
OutlineRect     ENDP

	END
