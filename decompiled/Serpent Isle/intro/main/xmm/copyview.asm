; Serpent Isle INTRO.EXE, resident segment 68 (file offsets 0x01250c to 0x012596, 138 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

	.MODEL  MEDIUM, C
	LOCALS

	PUBLIC  CopyView

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

; Copy one view's clip box onto another's, over the width and height both
; share. Row tables live in DGROUP, so they are read through ss once ds
; holds the source's pixels.
CopyView        PROC FAR src:WORD, dst:WORD
	LOCAL   srcLeft:WORD, dstLeft:WORD, dstRow:WORD
	USES    si, di, ds
	cld
	mov     bx, src
	mov     si, dst
	mov     ax, [bx].view_left
	mov     srcLeft, ax
	mov     cx, [si].view_left
	mov     dstLeft, cx
	mov     ax, [si].view_right
	sub     ax, cx
	inc     ax
	mov     dx, [bx].view_right
	sub     dx, [bx].view_left
	inc     dx
	cmp     dx, ax
	jbe     @@width
	mov     dx, ax
@@width:
	mov     cx, [si].view_bottom
	sub     cx, [si].view_top
	inc     cx
	mov     ax, [bx].view_bottom
	sub     ax, [bx].view_top
	inc     ax
	cmp     ax, cx
	jbe     @@height
	mov     ax, cx
@@height:
	mov     di, [si].view_rows
	mov     cx, [si].view_top
	shl     cx, 1
	add     cx, di
	mov     dstRow, cx
	mov     di, [bx].view_rows
	mov     cx, [bx].view_top
	shl     cx, 1
	add     cx, di
	mov     es, [si].view_seg
	mov     ds, [bx].view_seg
	mov     bx, cx
@@row:
	mov     si, ss:[bx]
	mov     di, dstRow
	mov     di, ss:[di]
	add     si, srcLeft
	add     di, dstLeft
	mov     cx, dx
	shr     cx, 1                       ; words, then the odd byte
	rep     movsw
	rcl     cx, 1
	rep     movsb
	add     dstRow, 2
	add     bx, 2
	dec     ax
	jne     @@row
	ret
CopyView        ENDP

	END
