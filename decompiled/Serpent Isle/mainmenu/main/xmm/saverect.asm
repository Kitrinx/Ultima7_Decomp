; Serpent Isle MAINMENU.EXE, resident segment 59 (file offsets 0x01844c to 0x0185ac, 352 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

; Holds saving a rectangle.

	.MODEL  MEDIUM
	.386
	LOCALS

	PUBLIC  _SaveRect

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

; Save a rectangle of a view into a buffer, clipped to the view.
_SaveRect PROC FAR
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
	push    cx
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
	mov     si, rect
	mov     bx, view
	xor     eax, eax
	mov     ax, [bx].view_seg
	mov     rowSeg, ax
	mov     eax, [bx].view_rows
	mov     rowPtr, eax
	xor     eax, eax
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
; start at the visible part of the save block
@@bottomDone:
	mov     ax, clipTop
	sub     ax, boxTop
	mul     word ptr boxWidth
	xor     edi, edi
	xor     edx, edx
	mov     di, ax
	mov     dx, clipLeft
	sub     dx, boxLeft
	add     edi, edx
	add     edi, buffer
	xor     eax, eax
	mov     es, ax
	mov     ax, clipTop
	shl     eax, 2
	add     rowPtr, eax
	mov     ds, rowSeg
; copy one view row into the buffer
@@row:
	mov     esi, rowPtr
	mov     esi, [esi]
	movzx   ecx, word ptr clipLeft
	add     esi, ecx
	movzx   ecx, word ptr cols
	push    ecx
	and     ecx, 3
	rep     movs byte ptr es:[edi], byte ptr [esi]
	pop     ecx
	shr     ecx, 2
	rep     movs dword ptr es:[edi], dword ptr [esi]
	add     dword ptr rowPtr, 4
	movzx   ecx, word ptr cols
	sub     edi, ecx
	movzx   ecx, word ptr boxWidth
	add     edi, ecx
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
_SaveRect ENDP

LOWLEVEL_TEXT ENDS

	END
