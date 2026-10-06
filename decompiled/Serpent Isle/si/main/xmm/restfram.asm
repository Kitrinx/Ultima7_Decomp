; Serpent Isle SI.EXE, one module of resident segment 58 (file offsets 0x023fa0 to 0x02415c, 444 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

; Holds restoring the pixels saved under a frame.

	.MODEL  MEDIUM
	.386
	LOCALS

	PUBLIC  _RestoreUnderFrame

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

; Copy the block saved from under a shape frame at x, y back onto a view,
; clipped to the view.
_RestoreUnderFrame PROC FAR
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
	shl     eax, 4
	mov     bx, word ptr shape
	add     eax, ebx
	mov     buffer, eax                 ; a slip: the linear shape lands in buffer
@@shapeFlat:
	xor     eax, eax
	mov     es, ax
	mov     edi, shape
	xor     ebx, ebx
	mov     bx, frameNum
	inc     bx
	shl     bx, 1
	shl     bx, 1
	cmp     es:[edi].shape_firstFrame, ebx
	jb      @@noFrame
	jne     @@haveFrame
@@noFrame:
	jmp     @@done
; box the frame covers, then clip its left edge
@@haveFrame:
	mov     esi, es:[edi+ebx]
	add     edi, esi
	mov     bx, view
	mov     ax, [bx].view_seg
	mov     rowSeg, ax
	mov     eax, [bx].view_rows
	mov     rowPtr, eax
	mov     ax, es:[edi].frame_right
	stc
	adc     ax, es:[edi].frame_left
	mov     boxWidth, ax
	mov     cols, ax
	mov     ax, es:[edi].frame_bottom
	stc
	adc     ax, es:[edi].frame_top
	mov     boxHeight, ax
	mov     rows, ax
	mov     ax, x
	sub     ax, es:[edi].frame_left
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
	sub     ax, es:[edi].frame_top
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
	add     ax, es:[edi].frame_right
	cmp     ax, [bx].view_left
	jl      @@offView
	sub     ax, [bx].view_right
	jle     @@rightDone
	sub     cols, ax
@@rightDone:
	mov     ax, y
	add     ax, es:[edi].frame_bottom
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
	mov     si, ax
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
_RestoreUnderFrame ENDP

LOWLEVEL_TEXT ENDS

	END
