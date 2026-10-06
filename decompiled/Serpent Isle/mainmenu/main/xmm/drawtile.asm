; Serpent Isle MAINMENU.EXE, one module of resident segment 59 (file offsets 0x0173e0 to 0x017481, 161 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

	.MODEL  MEDIUM
	.386
	LOCALS

	PUBLIC  _FillView, _FlatModeFlags

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
_FlatModeFlags      dw  0               ; bit 0: enter flat mode before each access

LOWLEVEL_TEXT SEGMENT DWORD PUBLIC USE16 'CODE'
	ASSUME  cs:LOWLEVEL_TEXT

; Fill a view's rectangle with one color, row by row through its row table.
_FillView PROC FAR
	ARG     view:WORD, color:BYTE
	LOCAL   left:DWORD = frame
	enter   frame, 0
	push    esi
	push    edi
	push    ds
	push    es
	push    fs
	push    gs
	pushf
	cld
	mov     ax, word ptr _FlatModeFlags
	and     ax, 1
	je      @@start
	call    far ptr _EnterFlatMode
@@start:
	mov     bx, view
	movzx   edx, word ptr [bx].view_right
	movzx   eax, word ptr [bx].view_left
	mov     left, eax
	sub     edx, eax
	inc     edx
	xor     esi, esi
	mov     si, [bx].view_bottom
	sub     si, [bx].view_top
	inc     si
	je      @@done
	mov     edi, [bx].view_rows
	push    esi
	xor     esi, esi
	mov     si, [bx].view_top
	shl     esi, 2
	add     edi, esi
	pop     esi
	xor     eax, eax
	mov     cx, [bx].view_seg
	mov     es, cx
	mov     ds, ax
	mov     al, color
	mov     ah, color
	push    ax
	push    ax
	pop     eax
; dwords first, then the odd bytes
@@row:
	push    edi
	mov     edi, [edi]
	add     edi, left
	mov     ecx, edx
	shr     ecx, 2
	rep     stos dword ptr es:[edi]
	mov     ecx, edx
	and     ecx, 3
	rep     stos byte ptr es:[edi]
	pop     edi
	add     edi, 4
	dec     esi
	jne     @@row
@@done:
	popf
	pop     gs
	pop     fs
	pop     es
	pop     ds
	pop     edi
	pop     esi
	leave
	ret
_FillView ENDP

LOWLEVEL_TEXT ENDS

	END
