; Serpent Isle SI.EXE, one module of resident segment 58 (file offsets 0x023e9c to 0x023f4e, 178 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

; Holds a shape frame's screen bounds.

	.MODEL  MEDIUM
	.386
	LOCALS

	PUBLIC  _GetFrameBounds

	EXTRN   _EnterFlatMode:FAR

; A rectangle, corners included.
RECTREC STRUC
rect_left   dw  ?
rect_top    dw  ?
rect_right  dw  ?
rect_bottom dw  ?
RECTREC ENDS

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

; Fill a rectangle with the screen bounds of a shape frame drawn at x, y.
; Returns -1, or 0 when the frame number is out of range.
_GetFrameBounds PROC FAR
	ARG     bounds:DWORD, x:WORD, y:WORD, shape:DWORD, frameNum:WORD, flags:WORD
	enter   0, 0
	push    edi
	push    esi
	push    ds
	push    es
	mov     ax, word ptr _FlatModeFlags
	and     ax, 1
	je      @@ready
	call    far ptr _EnterFlatMode
@@ready:
	xor     eax, eax
	mov     ds, ax
	mov     es, ax
	mov     ax, flags
	and     ax, 1
	mov     eax, shape
	jne     short @@flat
	xor     edx, edx
	push    dx
	push    eax
	pop     dx
	pop     eax
	shl     eax, 4
	add     eax, edx
@@flat:
	mov     esi, eax
	xor     ebx, ebx
	xor     eax, eax
	mov     bx, frameNum
	shl     bx, 1
	shl     bx, 1
	cmp     bx, word ptr [esi].shape_firstFrame
	jl      @@inRange
	xor     eax, eax
	jmp     short @@done
; left, top, right, bottom from the frame's four extents
@@inRange:
	add     bx, 4
	mov     eax, esi
	add     eax, [esi+ebx]
	mov     esi, eax
	mov     eax, bounds
	push    eax
	pop     di
	pop     es
	xor     eax, eax
	xor     ecx, ecx
	xor     edx, edx
	mov     cx, x
	mov     dx, y
	mov     ax, [esi].frame_right
	add     ax, cx
	mov     es:[di].rect_right, ax
	mov     ax, cx
	sub     ax, [esi].frame_left
	mov     es:[di].rect_left, ax
	mov     ax, dx
	sub     ax, [esi].frame_top
	mov     es:[di].rect_top, ax
	mov     ax, [esi].frame_bottom
	add     ax, dx
	mov     es:[di].rect_bottom, ax
	mov     eax, -1
@@done:
	pop     es
	pop     ds
	pop     esi
	pop     edi
	leave
	ret
_GetFrameBounds ENDP

LOWLEVEL_TEXT ENDS

	END
