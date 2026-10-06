; Serpent Isle MAINMENU.EXE, resident segment 59 (file offsets 0x018274 to 0x018449, 469 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

; Holds saving the pixels under a frame.

	.MODEL  MEDIUM
	.386
	LOCALS

	PUBLIC  _SaveUnderFrame

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

	.DATA
	EXTRN   _FlatModeFlags:WORD

LOWLEVEL_TEXT SEGMENT DWORD PUBLIC USE16 'CODE'
	ASSUME  cs:LOWLEVEL_TEXT

; Save the part of a view a shape frame at x, y will cover into a buffer,
; clipped to the view.
_SaveUnderFrame PROC FAR
	ARG     view:WORD, buffer:DWORD, x:WORD, y:WORD, shape:DWORD, frameNum:WORD, flags:WORD
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
@@bufFlat:
	xor     eax, eax
	mov     ax, flags
	and     ah, 1
	jne     @@shapeFlat
	xor     ebx, ebx
	mov     ax, word ptr shape+2
	shl     ax, 4                       ; a slip: 16 bits lose the top of the address
	mov     bx, word ptr shape
	add     eax, ebx
	mov     shape, eax
@@shapeFlat:
	xor     eax, eax
	mov     es, ax
	mov     esi, shape
	xor     ebx, ebx
	mov     bx, frameNum
	inc     bx
	shl     bx, 1
	shl     bx, 1
	cmp     word ptr es:[esi].shape_firstFrame, bx
	jb      @@noFrame
	jne     @@haveFrame
@@noFrame:
	jmp     @@done
; box the frame covers, then clip its left edge
@@haveFrame:
	mov     edi, es:[esi+ebx]
	add     esi, edi
	mov     bx, view
	xor     eax, eax
	mov     ax, [bx].view_seg
	mov     rowSeg, ax
	mov     eax, [bx].view_rows
	mov     rowPtr, eax
	xor     eax, eax
	mov     ax, es:[esi].frame_right
	stc
	adc     ax, es:[esi].frame_left
	mov     boxWidth, ax
	mov     cols, ax
	xor     eax, eax
	mov     ax, es:[esi].frame_bottom
	stc
	adc     ax, es:[esi].frame_top
	mov     boxHeight, ax
	mov     rows, ax
	xor     eax, eax
	mov     ax, x
	sub     ax, es:[esi].frame_left
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
	xor     eax, eax
	mov     ax, y
	sub     ax, es:[esi].frame_top
	cmp     ax, [bx].view_bottom
	jle     @@topIn
@@outside:
	jmp     @@done
@@topIn:
	mov     boxTop, ax
	mov     clipTop, ax
	xor     eax, eax
	mov     ax, [bx].view_top
	sub     ax, clipTop
	jl      @@topDone
	sub     rows, ax
	mov     ax, [bx].view_top
	mov     clipTop, ax
; clip the right and bottom edges
@@topDone:
	xor     eax, eax
	mov     ax, x
	add     ax, es:[esi].frame_right
	cmp     ax, [bx].view_left
	jl      @@offView
	sub     ax, [bx].view_right
	jle     @@rightDone
	sub     cols, ax
@@rightDone:
	xor     eax, eax
	mov     ax, y
	add     ax, es:[esi].frame_bottom
	cmp     ax, [bx].view_top
@@offView:
	jl      @@done
	sub     ax, [bx].view_bottom
	jle     @@bottomDone
	sub     rows, ax
; start at the visible part of the save block
@@bottomDone:
	xor     eax, eax
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
_SaveUnderFrame ENDP

LOWLEVEL_TEXT ENDS

	END
