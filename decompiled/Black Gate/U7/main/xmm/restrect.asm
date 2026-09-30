; Black Gate U7.EXE, one module of resident segment 27 (file offsets 0x016344 to 0x0164a9, 357 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.
; Segment 27 joins 25 modules' code as LOWLEVEL_TEXT, in link order and doubleword aligned.
; This one starts at 15F0h; its own empty segment 165 fixes its link position.
; Holds restoring a saved rectangle.

	.MODEL  MEDIUM
	.386
	LOCALS

	PUBLIC  _RestoreRect

	EXTRN   _EnterFlatMode:FAR

; A view: the segment its row addresses count from, the flat address of its
; table of row addresses, and its clip box, corners included.
VIEWREC STRUC
view_seg    dw  ?
view_rows   dd  ?
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

	.DATA
	EXTRN   _FlatModeFlags:WORD

LOWLEVEL_TEXT SEGMENT DWORD PUBLIC USE16 'CODE'
	ASSUME  cs:LOWLEVEL_TEXT

; Copy a block saved from a rectangle back onto a view, clipped to the view.
_RestoreRect PROC FAR
	ARG     view:WORD, buffer:DWORD, rect:WORD, flags:WORD
	LOCAL   boxTop:WORD, boxLeft:WORD, boxWidth:WORD, boxHeight:WORD, clipTop:WORD, \
		clipLeft:WORD, cols:WORD, rows:WORD, rowSeg:WORD, rowPtr:DWORD = frame
	enter   frame, 0
	push    esi
	push    edi
	push    ds
	push    es
	pushf
	cld
	mov     ax, word ptr _FlatModeFlags
	and     ax, 1
	je      @@ready
	call    far ptr _EnterFlatMode
@@ready:
	xor     eax, eax
	mov     ax, flags
	and     ax, 1
	jne     @@bufFlat
	xor     ebx, ebx
	mov     ax, word ptr buffer+2
	shl     eax, 4
	mov     bx, word ptr buffer
	add     eax, ebx
	mov     buffer, eax
; rectangle size, then clip its left edge
@@bufFlat:
	mov     ax, ds
	mov     es, ax
	mov     di, rect
	mov     bx, view
	xor     eax, eax
	mov     ax, [bx].view_seg
	mov     rowSeg, ax
	mov     eax, [bx].view_rows
	mov     rowPtr, eax
	xor     eax, eax
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
	xor     esi, esi
	xor     edx, edx
	mov     esi, eax
	mov     dx, clipLeft
	sub     dx, boxLeft
	add     esi, edx
	add     esi, buffer
	xor     eax, eax
	mov     ds, ax
	mov     ax, clipTop
	shl     eax, 2
	add     rowPtr, eax
	mov     es, rowSeg
; copy one row back onto the view
@@row:
	mov     edi, rowPtr
	mov     edi, [edi]
	movzx   ecx, word ptr clipLeft
	add     edi, ecx
	movzx   ecx, word ptr cols
	push    ecx
	and     ecx, 3
	rep     movs byte ptr es:[edi], byte ptr [esi]
	pop     ecx
	shr     ecx, 2
	rep     movs dword ptr es:[edi], dword ptr [esi]
	add     dword ptr rowPtr, 4
	movzx   ecx, word ptr cols
	sub     esi, ecx
	movzx   ecx, word ptr boxWidth
	add     esi, ecx
	dec     word ptr rows
	jne     @@row
@@done:
	popf
	pop     es
	pop     ds
	pop     edi
	pop     esi
	leave
	ret
_RestoreRect ENDP

LOWLEVEL_TEXT ENDS

	END
