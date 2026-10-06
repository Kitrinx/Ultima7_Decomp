; Serpent Isle INTRO.EXE, resident segment 65 (file offsets 0x012118 to 0x0121a7, 143 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

	.MODEL  MEDIUM, C
	LOCALS

	PUBLIC  FillRect

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

fillColor   dw  0                       ; the color in both bytes
rowsLeft    dw  0
unusedTop   dw  0
unusedBottom dw 0
left        dw  0
right       dw  0
topRow      dw  0                       ; clip rows, as row table offsets
bottomRow   dw  0

	.CODE

; Fill the rectangle from x0, y0 to x1, y1 with one color, clipped to the view.
; The row table is read through bp, so the arguments are copied out first.
FillRect        PROC FAR view:WORD, x0:WORD, y0:WORD, x1:WORD, y1:WORD, color:WORD
	USES    si, di
	mov     bx, view
	mov     es, [bx].view_seg
	mov     ax, color
	mov     ah, al
	mov     fillColor, ax
	mov     cx, x0
	cmp     cx, [bx].view_right
	jg      @@clipped
	cmp     cx, [bx].view_left
	jge     @@clipX1
	mov     cx, [bx].view_left
@@clipX1:
	mov     left, cx
	mov     cx, x1
	cmp     cx, [bx].view_left
	jl      @@clipped
	cmp     cx, [bx].view_right
	jle     @@clipY
	mov     cx, [bx].view_right
@@clipY:
	mov     right, cx
	mov     ax, [bx].view_top
	shl     ax, 1
	mov     topRow, ax
	mov     ax, [bx].view_bottom
	shl     ax, 1
	mov     bottomRow, ax
	mov     si, y0
	mov     ax, y1
	sub     ax, si
	mov     rowsLeft, ax
	shl     si, 1
	mov     bp, [bx].view_rows
	jmp     short @@row
@@clipped:
	jmp     short @@done
@@row:
	cmp     si, topRow
	jl      @@nextRow
	cmp     si, bottomRow
	jg      @@done
	mov     di, left
	mov     cx, right
	sub     cx, di
	inc     cx
	add     di, [bp+si]
	mov     ax, fillColor
	shr     cx, 1                       ; words, then the odd byte
	rep     stosw
	rcl     cx, 1
	rep     stosb
@@nextRow:
	add     si, 2
	dec     rowsLeft
	jns     @@row
@@done:
	ret
FillRect        ENDP

	END
