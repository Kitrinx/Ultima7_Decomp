; Serpent Isle SI.EXE, resident segment 58 (file offsets 0x022c34 to 0x022e63, 559 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

; Holds the depth buffer used to hide shapes behind others.

	.MODEL  MEDIUM
	.386
	LOCALS

	PUBLIC  _RaiseDepth, _RaiseDepthRun, _RaiseDepthRect, _IsBelowDepth, _IsBelowDepthCorners
	PUBLIC  _IsShapeHidden, _IsDepthBlockSet

	EXTRN   _EnterFlatMode:FAR

	.DATA
	EXTRN   _OcclusionRows:DWORD
	EXTRN   _FlatModeFlags:WORD
	EXTRN   _OcclusionBoxX:WORD
	EXTRN   _OcclusionBoxY:WORD
	EXTRN   _OcclusionBoxZ:WORD
	EXTRN   _OcclusionBoxWidth:WORD
	EXTRN   _OcclusionBoxLength:WORD
	EXTRN   _OcclusionBoxHeight:WORD

LOWLEVEL_TEXT   SEGMENT DWORD PUBLIC USE16 'CODE'
	ASSUME  cs:LOWLEVEL_TEXT

; Raise one cell of the 128-byte-wide depth buffer to depth.
_RaiseDepth PROC FAR
	ARG     cell:DWORD, depth:WORD
	enter   0, 0
	push    ds
	push    bx
	mov     ax, word ptr _FlatModeFlags
	and     ax, 1
	je      short @@start
	call    far ptr _EnterFlatMode
@@start:
	xor     eax, eax
	mov     ds, ax
	mov     ebx, cell
	mov     ax, depth
	cmp     al, [ebx]
	jle     short @@done
	mov     [ebx], al
@@done:
	pop     bx
	pop     ds
	leave
	ret
_RaiseDepth ENDP

; Raise a horizontal run of depth cells to depth.
_RaiseDepthRun PROC FAR
	ARG     cell:DWORD, depth:WORD, run:WORD
	enter   0, 0
	push    ds
	push    bx
	push    cx
	mov     ax, word ptr _FlatModeFlags
	and     ax, 1
	je      short @@start
	call    far ptr _EnterFlatMode
@@start:
	xor     eax, eax
	mov     ds, ax
	mov     ebx, cell
	mov     ax, depth
	mov     cx, run
@@cell:
	cmp     al, [ebx]
	jle     short @@next
	mov     [ebx], al
@@next:
	inc     ebx
	loop    @@cell
	pop     cx
	pop     bx
	pop     ds
	leave
	ret
_RaiseDepthRun ENDP

; Raise a rectangle of depth cells to depth, one 128-byte row at a time.
_RaiseDepthRect PROC FAR
	ARG     cell:DWORD, depth:WORD, cols:WORD, rows:WORD
	enter   0, 0
	push    ds
	push    bx
	push    cx
	push    dx
	mov     ax, word ptr _FlatModeFlags
	and     ax, 1
	je      short @@start
	call    far ptr _EnterFlatMode
@@start:
	xor     eax, eax
	mov     ds, ax
	mov     ax, depth
	mov     dx, rows
; take this row and step the pointer to the next
@@row:
	mov     ebx, cell
	push    ebx
	add     ebx, 128
	mov     cell, ebx
	pop     ebx
	mov     cx, cols
@@cell:
	cmp     al, [ebx]
	jle     short @@next
	mov     [ebx], al
@@next:
	inc     ebx
	loop    @@cell
	dec     dx
	jne     @@row
	pop     dx
	pop     cx
	pop     bx
	pop     ds
	leave
	ret
_RaiseDepthRect ENDP

; Return 1 when depth is below the depth cell, else 0.
_IsBelowDepth PROC FAR
	ARG     cell:DWORD, depth:WORD
	enter   0, 0
	push    ds
	push    bx
	push    cx
	mov     ax, word ptr _FlatModeFlags
	and     ax, 1
	je      short @@start
	call    far ptr _EnterFlatMode
@@start:
	xor     eax, eax
	mov     ds, ax
	mov     ebx, cell
	mov     cx, depth
	cmp     cl, [ebx]
	jge     short @@done
	inc     ax
@@done:
	pop     cx
	pop     bx
	pop     ds
	leave
	ret
_IsBelowDepth ENDP

; Return 1 when depth is below all four corner cells of a box
; cols across and rows down, else 0.
_IsBelowDepthCorners PROC FAR
	ARG     cell:DWORD, depth:WORD, cols:WORD, rows:WORD
	enter   0, 0
	push    ds
	push    bx
	push    cx
	push    dx
	push    edi
	mov     ax, word ptr _FlatModeFlags
	and     ax, 1
	je      short @@start
	call    far ptr _EnterFlatMode
@@start:
	xor     eax, eax
	mov     ds, ax
	xor     edi, edi
	mov     di, cols
	mov     ebx, cell
	mov     cx, depth
	cmp     cl, [ebx]
	jge     short @@done
	cmp     cl, [ebx+edi]
	jge     short @@done
	mov     ax, 128
	mul     word ptr rows
	add     ebx, eax
	xor     eax, eax
	cmp     cl, [ebx]
	jge     short @@done
	cmp     cl, [ebx+edi]
	jge     short @@done
	inc     ax
@@done:
	pop     edi
	pop     dx
	pop     cx
	pop     bx
	pop     ds
	leave
	ret
_IsBelowDepthCorners ENDP

; Return 1 when the box described by the occlusion globals lies below
; the occlusion depth at its corners and centre, else 0.
_IsShapeHidden PROC FAR
	push    es
	push    ds
	push    bx
	push    cx
	push    dx
	push    edi
	mov     ax, word ptr _FlatModeFlags
	and     ax, 1
	je      short @@start
	call    far ptr _EnterFlatMode
@@start:
	mov     ax, ds
	mov     es, ax
	mov     esi, dword ptr _OcclusionRows
	xor     eax, eax
	mov     ds, ax
	xor     edi, edi
	mov     di, word ptr es:_OcclusionBoxX
	mov     ax, word ptr es:_OcclusionBoxY
	shl     eax, 2
	add     esi, eax
	mov     ebx, [esi]
	xor     eax, eax
	mov     cx, word ptr es:_OcclusionBoxHeight
	push    cx
	shr     cx, 1
	mov     word ptr es:_OcclusionBoxHeight, cx
	pop     cx
	add     cx, word ptr es:_OcclusionBoxZ
	cmp     cl, [ebx+edi]
	jge     short @@done
	add     di, word ptr es:_OcclusionBoxWidth
	cmp     cl, [ebx+edi]
	jge     short @@done
	mov     ax, word ptr es:_OcclusionBoxLength
	shr     ax, 1
	sub     word ptr es:_OcclusionBoxLength, ax
	shl     ax, 2
	add     esi, eax
	mov     ebx, [esi]
	mov     ax, word ptr es:_OcclusionBoxWidth
	shr     ax, 1
	sub     di, ax
	xor     eax, eax
	sub     cx, word ptr es:_OcclusionBoxHeight
	cmp     cl, [ebx+edi]
	jge     short @@done
	mov     ax, word ptr es:_OcclusionBoxLength
	shl     ax, 2
	add     esi, eax
	mov     ebx, [esi]
	mov     di, word ptr es:_OcclusionBoxX
	xor     eax, eax
	mov     cx, word ptr es:_OcclusionBoxZ
	cmp     cl, [ebx+edi]
	jge     short @@done
	add     di, word ptr es:_OcclusionBoxWidth
	cmp     cl, [ebx+edi]
	jge     short @@done
	inc     ax
@@done:
	pop     edi
	pop     dx
	pop     cx
	pop     bx
	pop     ds
	pop     es
	ret
_IsShapeHidden ENDP

; Return 1 when all four cells of a 2x2 depth block are nonzero, else 0.
_IsDepthBlockSet PROC FAR
	ARG     cell:DWORD
	enter   0, 0
	push    ds
	push    bx
	push    cx
	push    dx
	push    edi
	mov     ax, word ptr _FlatModeFlags
	and     ax, 1
	je      short @@start
	call    far ptr _EnterFlatMode
@@start:
	xor     eax, eax
	mov     ds, ax
	mov     ebx, cell
	mov     cx, [ebx]
	or      cl, cl
	je      short @@done
	or      ch, ch
	je      short @@done
	add     ebx, 128
	mov     cx, [ebx]
	or      cl, cl
	je      short @@done
	or      ch, ch
	je      short @@done
	inc     ax
@@done:
	pop     edi
	pop     dx
	pop     cx
	pop     bx
	pop     ds
	leave
	ret
_IsDepthBlockSet ENDP

LOWLEVEL_TEXT   ENDS

	END
