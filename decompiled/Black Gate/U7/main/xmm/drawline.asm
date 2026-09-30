; Black Gate U7.EXE, resident segment 174 (file offsets 0x03f95e to 0x03fb86, 552 bytes).
; Turbo Assembler 2.51 /mx /m2 rebuilds it byte for byte.
; Folder inferred from compiler flags and link order.

	.MODEL  MEDIUM
	.386
	LOCALS

	PUBLIC  _DrawLine

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

	.CODE

; Draw a line from x0, y0 to x1, y1 in one color, clipped to the view, stepping Bresenham style.
_DrawLine PROC FAR
	ARG     view:WORD, x0:WORD, y0:WORD, x1:WORD, y1:WORD, color:BYTE
	LOCAL   rowStep:DWORD, xSpan:WORD, ySpan:WORD, upward:WORD, pitch:DWORD = frame
	enter   frame, 0
	push    esi
	push    edi
	push    es
	push    ds
	pushf
	cld
	push    0
	pop     es
	mov     si, view
	mov     ebx, [si].view_rows
	mov     eax, dword ptr es:[ebx+4]   ; the second row's address less the first's
	sub     eax, dword ptr es:[ebx]
	mov     pitch, eax
	mov     ax, x0
	mov     bx, x1
	mov     cx, y0
	mov     dx, y1
	cmp     bx, ax
	jge     @@xOrdered
	xchg    bx, ax
	xchg    cx, dx
	mov     x0, ax
	mov     x1, bx
	mov     y0, cx
	mov     y1, dx
@@xOrdered:
	push    ax
	mov     eax, pitch
	cmp     dx, cx
	jge     @@rowsDown
	xchg    cx, dx
	neg     eax
	mov     upward, 1
	jmp     @@strideSet
@@rowsDown:
	mov     upward, 0
@@strideSet:
	mov     rowStep, eax
	pop     ax
	cmp     bx, [si].view_left
	jl      @@done
	cmp     ax, [si].view_right
	jg      @@done
	cmp     dx, [si].view_top
	jl      @@done
	cmp     cx, [si].view_bottom
	jg      @@done
	cmp     bx, [si].view_right
	jle     @@x1Clipped
	mov     bx, x1
	sub     bx, x0
	mov     dx, y1
	sub     dx, y0
	mov     ax, [si].view_right
	sub     ax, x0
	imul    dx
	idiv    bx
	add     ax, y0
	mov     y1, ax
	mov     ax, [si].view_right
	mov     x1, ax
@@x1Clipped:
	mov     ax, x0
	cmp     ax, [si].view_left
	jge     @@x0Clipped
	mov     bx, x1
	sub     bx, x0
	mov     dx, y1
	sub     dx, y0
	mov     ax, [si].view_left
	sub     ax, x0
	imul    dx
	idiv    bx
	add     ax, y0
	mov     y0, ax
	mov     ax, [si].view_left
	mov     x0, ax
@@x0Clipped:
	mov     ax, y0
	mov     bx, y1
	mov     cx, x0
	mov     dx, x1
	test    upward, 0FFh
	je      @@unswapped
	xchg    bx, ax
	xchg    cx, dx
	mov     y0, ax
	mov     y1, bx
	mov     x0, cx
	mov     x1, dx
@@unswapped:
	cmp     bx, [si].view_top
	jl      @@done
	cmp     ax, [si].view_bottom
	jg      @@done
	cmp     bx, [si].view_bottom
	jle     @@y1Clipped
	sub     bx, ax
	mov     dx, x1
	sub     dx, x0
	mov     ax, [si].view_bottom
	sub     ax, y0
	imul    dx
	idiv    bx
	add     ax, x0
	mov     x1, ax
	mov     ax, [si].view_bottom
	mov     y1, ax
@@y1Clipped:
	mov     ax, y0
	mov     bx, y1
	cmp     ax, [si].view_top
	jge     @@y0Clipped
	mov     bx, y1
	sub     bx, y0
	mov     dx, x1
	sub     dx, x0
	mov     ax, [si].view_top
	sub     ax, y0
	imul    dx
	idiv    bx
	add     ax, x0
	mov     x0, ax
	mov     ax, [si].view_top
	mov     y0, ax
@@y0Clipped:
	mov     bx, y1
	sub     bx, y0
	mov     ySpan, bx
	test    upward, 0FFh
	je      @@backInOrder
	mov     ax, y0
	mov     bx, y1
	mov     cx, x0
	mov     dx, x1
	xchg    bx, ax
	xchg    cx, dx
	mov     y0, ax
	mov     y1, bx
	mov     x0, cx
	mov     x1, dx
@@backInOrder:
	mov     bx, x1
	sub     bx, x0
	mov     xSpan, bx
	mov     esi, [si].view_rows
	movzx   ecx, y0
	shl     cx, 2
	add     ecx, esi
	movzx   eax, x0
	add     eax, dword ptr es:[ecx]
	mov     edi, eax
	mov     al, color
	xor     ecx, ecx
	mov     bx, x0
	cmp     bx, x1
	je      @@vertical
	mov     bx, y0
	cmp     bx, y1
	je      @@horizontal
	mov     bx, xSpan
	mov     dx, ySpan
	cmp     dx, bx
	jg      @@steep
	mov     si, bx
	inc     si
@@shallowStep:
	stos    byte ptr es:[edi]
	dec     si
	je      @@done
	add     cx, dx
	cmp     cx, bx
	jl      @@shallowStep
	add     edi, rowStep
	sub     cx, bx
	jmp     @@shallowStep
@@steep:
	mov     si, dx
	inc     si
	dec     rowStep
@@steepStep:
	stos    byte ptr es:[edi]
	add     edi, rowStep
	dec     si
	je      @@done
	add     cx, bx
	cmp     cx, dx
	jl      @@steepStep
	inc     di
	sub     cx, dx
	jmp     @@steepStep
@@vertical:
	mov     cx, ySpan
	inc     cx
	mov     ebx, rowStep
	dec     ebx
@@verticalStep:
	stos    byte ptr es:[edi]
	add     edi, ebx
	loop    @@verticalStep
	jmp     @@done
@@horizontal:
	mov     cx, xSpan
	inc     cx
	mov     ah, al
	push    ax
	push    ax
	pop     eax
	mov     bx, cx
	and     bx, 3
	shr     cx, 2
	rep     stos dword ptr es:[edi]
	mov     cx, bx
	rep     stos byte ptr es:[edi]
@@done:
	popf
	pop     ds
	pop     es
	pop     edi
	pop     esi
	leave
	ret
_DrawLine ENDP

	END
