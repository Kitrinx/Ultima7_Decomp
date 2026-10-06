; Serpent Isle SI.EXE, resident segment 168 (file offsets 0x03f500 to 0x03f5d7, 215 bytes).
; Turbo Assembler 2.51 /mx /m2 rebuilds it byte for byte.

	.MODEL  MEDIUM
	.386
	LOCALS

	PUBLIC  _FillRectangle

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

; Fill the rectangle from x0, y0 to x1, y1 with one color, clipped to the view.
_FillRectangle PROC FAR
	ARG     view:WORD, x0:WORD, y0:WORD, x1:WORD, y1:WORD, color:BYTE
	LOCAL   rowSkip:DWORD = frame
	enter   frame, 0
	push    esi
	push    edi
	push    es
	push    ds
	pushf
	cld
	mov     bx, view
	mov     cx, x0
	cmp     cx, [bx].view_right
	jg      @@done
	cmp     cx, [bx].view_left
	jge     @@clipX1
	mov     cx, [bx].view_left
	mov     x0, cx
@@clipX1:
	mov     cx, x1
	cmp     cx, [bx].view_left
	jl      @@done
	cmp     cx, [bx].view_right
	jle     @@clipY0
	mov     cx, [bx].view_right
	mov     x1, cx
@@clipY0:
	mov     cx, y0
	cmp     cx, [bx].view_bottom
	jg      @@done
	cmp     cx, [bx].view_top
	jge     @@clipY1
	mov     cx, [bx].view_top
	mov     y0, cx
@@clipY1:
	mov     cx, y1
	cmp     cx, [bx].view_top
	jl      @@done
	cmp     cx, [bx].view_bottom
	jle     @@fill
	mov     cx, [bx].view_bottom
	mov     y1, cx
@@fill:
	push    0
	pop     es
	mov     dx, x1
	sub     dx, x0
	inc     dx
	mov     ax, dx
	and     ax, 3
	mov     gs, ax
	mov     ax, dx
	shr     ax, 2
	mov     fs, ax
	mov     esi, [bx].view_rows
	mov     ecx, dword ptr es:[esi+4]
	sub     ecx, dword ptr es:[esi]
	sub     cx, dx
	mov     rowSkip, ecx
	movzx   ecx, y0
	shl     cx, 2
	add     ecx, esi
	movsx   eax, x0
	add     eax, dword ptr es:[ecx]
	mov     edi, eax
	mov     si, y1
	sub     si, y0
	inc     si
	mov     edx, rowSkip
	xor     ecx, ecx
	mov     al, color
	mov     ah, al
	push    ax
	push    ax
	pop     eax
@@row:
	mov     cx, fs
	rep     stos dword ptr es:[edi]
	mov     cx, gs
	rep     stos byte ptr es:[edi]
	add     edi, edx
	dec     si
	jne     @@row
@@done:
	popf
	pop     ds
	pop     es
	pop     edi
	pop     esi
	leave
	ret
_FillRectangle ENDP

	END
