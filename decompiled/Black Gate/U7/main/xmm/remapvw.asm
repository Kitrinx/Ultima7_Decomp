; Black Gate U7.EXE, one module of resident segment 27 (file offsets 0x018ac0 to 0x018b51, 145 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.
; Segment 27 joins 25 modules' code as LOWLEVEL_TEXT, in link order and doubleword aligned.
; This one starts at 3D6Ch; its own empty segment 179 fixes its link position.
; Holds remapping a view's pixels through a color table.

	.MODEL  MEDIUM
	.386
	LOCALS

	PUBLIC  _RemapView

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

; Pass every pixel of the view's rectangle through a 256-byte color table, in place.
_RemapView PROC FAR
	ARG     view:WORD, colors:DWORD
	LOCAL   firstCol:DWORD = frame
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
	je      @@remap
	call    far ptr _EnterFlatMode
@@remap:
	mov     bx, view
	movzx   edx, word ptr [bx].view_right
	movzx   eax, word ptr [bx].view_left
	mov     firstCol, eax
	sub     dx, ax
	inc     dx
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
	mov     es, ax
	mov     ds, ax
	mov     ebx, colors
@@nextRow:
	xor     eax, eax
	push    edi
	mov     edi, [edi]
	add     edi, firstCol
	mov     cx, dx
@@nextPixel:
	mov     al, [edi]
	xlat    byte ptr [ebx]
	stos    byte ptr es:[edi]
	dec     cx
	jne     @@nextPixel
	pop     edi
	add     edi, 4
	dec     esi
	jne     @@nextRow
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
_RemapView ENDP

LOWLEVEL_TEXT ENDS

	END
