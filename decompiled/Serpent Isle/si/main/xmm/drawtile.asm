; Serpent Isle SI.EXE, one module of resident segment 58 (file offsets 0x0231c0 to 0x0232fd, 317 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

; Its data is DS:6EF4-6F2C, between drawbuf.c's and freexmm.c's; DrawTile alone reads the table.
; Holds tile drawing and view fills, with the screen row table.

	.MODEL  MEDIUM
	.386
	LOCALS

	PUBLIC  _DrawTile, _FillView, _TileRowOffsets, _ViewportFirstRow, _FlatModeFlags

	EXTRN   _EnterFlatMode:FAR

SCREEN_WIDTH    EQU 320

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
; the screen offset of each row of tiles, 8 lines apart
_TileRowOffsets LABEL WORD
	dw      0, 2560, 5120, 7680, 10240, 12800, 15360, 17920, 20480, 23040
	dw      25600, 28160, 30720, 33280, 35840, 38400, 40960, 43520, 46080, 48640
	dw      51200, 53760, 56320, 58880, 61440
_ViewportFirstRow   dd  0               ; flat address of the screen's first row
_FlatModeFlags      dw  0               ; bit 0: enter flat mode before each access

LOWLEVEL_TEXT SEGMENT DWORD PUBLIC USE16 'CODE'
	ASSUME  cs:LOWLEVEL_TEXT

; Copy an 8x8 tile to the screen at tile column x and tile row y.
_DrawTile PROC FAR
	ARG     tile:DWORD, x:WORD, y:WORD
	enter   0, 0
	push    esi
	push    edi
	push    ds
	push    es
	pushf
	cld
	mov     ax, word ptr _FlatModeFlags
	and     ax, 1
	je      @@start
	call    far ptr _EnterFlatMode
@@start:
	movzx   eax, word ptr x
	shl     eax, 3
	movzx   ebx, word ptr y
	shl     ebx, 1
	movzx   edi, word ptr _TileRowOffsets[bx]
	mov     ebx, dword ptr _ViewportFirstRow
	add     edi, ebx
	add     edi, eax
	mov     esi, tile
	mov     ebx, SCREEN_WIDTH - 8
	xor     eax, eax
	mov     es, ax
	mov     ds, ax
	REPT    7
	movs    dword ptr es:[edi], dword ptr [esi]
	movs    dword ptr es:[edi], dword ptr [esi]
	add     edi, ebx
	ENDM
	movs    dword ptr es:[edi], dword ptr [esi]
	movs    dword ptr es:[edi], dword ptr [esi]
	popf
	pop     es
	pop     ds
	pop     edi
	pop     esi
	leave
	ret
_DrawTile ENDP

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
