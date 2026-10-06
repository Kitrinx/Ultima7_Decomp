; Serpent Isle SI.EXE, one module of resident segment 58 (file offsets 0x023d0c to 0x023e05, 249 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

; Holds copying one view to another.

	.MODEL  MEDIUM
	.386
	LOCALS

	PUBLIC  _CopyView

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

	.DATA
	EXTRN   _FlatModeFlags:WORD

LOWLEVEL_TEXT SEGMENT DWORD PUBLIC USE16 'CODE'
	ASSUME  cs:LOWLEVEL_TEXT

; Copy one view onto another, row by row, over the width and height
; both views share. Rows start at the source view's top line.
_CopyView PROC FAR
	ARG     src:WORD, dst:WORD
	LOCAL   cols:WORD, rows:WORD, srcLeft:DWORD, dstLeft:DWORD = frame
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
	mov     bx, src
	movzx   eax, word ptr [bx].view_right
	movzx   ecx, word ptr [bx].view_left
	mov     srcLeft, ecx
	sub     ax, cx
	inc     ax
	je      @@done
	mov     cols, ax
	xor     esi, esi
	mov     si, [bx].view_bottom
	sub     si, [bx].view_top
	inc     si
	mov     rows, si
	mov     di, dst
	movzx   eax, word ptr [di].view_right
	movzx   ecx, word ptr [di].view_left
	mov     dstLeft, ecx
	sub     ax, cx
	inc     ax
	cmp     ax, cols
	jge     @@colsSet
	mov     cols, ax
@@colsSet:
	xor     esi, esi
	mov     si, [di].view_bottom
	sub     si, [di].view_top
	inc     si
	cmp     si, rows
	jge     @@rowsSet
	mov     rows, si
; point both row tables at the source's top row
@@rowsSet:
	mov     dx, rows
	cmp     dx, 0
	je      @@done
	mov     edi, [di].view_rows
	push    esi
	xor     esi, esi
	mov     si, [bx].view_top
	shl     esi, 2
	add     edi, esi
	pop     esi
	mov     esi, [bx].view_rows
	push    edi
	xor     edi, edi
	mov     di, [bx].view_top
	shl     edi, 2
	add     esi, edi
	pop     edi
	xor     eax, eax
	mov     ds, ax
	mov     es, ax
	xor     ebx, ebx
	mov     bx, cols
; whole dwords, then the odd bytes
@@row:
	push    edi
	push    esi
	mov     edi, [edi]
	add     edi, dstLeft
	mov     esi, [esi]
	add     esi, srcLeft
	movzx   ecx, bx
	shr     ecx, 2
	rep     movs dword ptr es:[edi], dword ptr [esi]
	movzx   ecx, bx
	and     ecx, 3
	rep     movs byte ptr es:[edi], byte ptr [esi]
	pop     esi
	pop     edi
	add     edi, 4
	add     esi, 4
	dec     dx
	jne     @@row
@@done:
	popf
	pop     es
	pop     ds
	pop     edi
	pop     esi
	leave
	ret
_CopyView ENDP

LOWLEVEL_TEXT ENDS

	END
