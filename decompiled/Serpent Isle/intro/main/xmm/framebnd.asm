; Serpent Isle INTRO.EXE, resident segment 16 (file offsets 0x00c896 to 0x00c91e, 136 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

	.MODEL  MEDIUM, PASCAL
	LOCALS

	PUBLIC  GETFRAMEBOUNDS

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

	.CODE

; Fill a rectangle with the screen bounds of a shape frame drawn at x, y.
; Returns -1, or 0 when the frame number is out of range.
GETFRAMEBOUNDS  PROC FAR bounds:DWORD, x:WORD, y:WORD, shape:DWORD, frameNum:WORD
	USES    ds, es, si, di
	lds     si, shape
	mov     bx, frameNum
	mov     ax, ds                      ; the shape's linear address in dx:ax
	xor     dx, dx
	shl     ax, 1
	rcl     dx, 1
	shl     ax, 1
	rcl     dx, 1
	shl     ax, 1
	rcl     dx, 1
	shl     ax, 1
	rcl     dx, 1
	add     ax, si
	adc     dx, 0
	shl     bx, 1
	shl     bx, 1
	cmp     bx, word ptr [si].shape_firstFrame
	jl      @@inRange
	xor     ax, ax
	jmp     short @@done
@@inRange:
	add     bx, 4
	add     ax, [bx+si]                 ; plus the frame's offset
	adc     dx, [bx+si+2]
	mov     si, ax                      ; back to a normalized ds:si
	and     si, 0Fh
	shr     dx, 1
	rcr     ax, 1
	shr     dx, 1
	rcr     ax, 1
	shr     dx, 1
	rcr     ax, 1
	shr     dx, 1
	rcr     ax, 1
	mov     ds, ax
; left, top, right, bottom from the frame's four extents
	les     di, bounds
	mov     cx, x
	mov     dx, y
	mov     ax, [si].frame_right
	add     ax, cx
	mov     es:[di].rect_right, ax
	mov     ax, cx
	sub     ax, [si].frame_left
	mov     es:[di].rect_left, ax
	mov     ax, dx
	sub     ax, [si].frame_top
	mov     es:[di].rect_top, ax
	mov     ax, [si].frame_bottom
	add     ax, dx
	mov     es:[di].rect_bottom, ax
	mov     ax, -1
@@done:
	ret
GETFRAMEBOUNDS  ENDP

	END
