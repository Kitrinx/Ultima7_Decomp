; Black Gate U7.EXE, one module of resident segment 27 (file offsets 0x016138 to 0x016188, 80 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.
; Segment 27 joins 25 modules' code as LOWLEVEL_TEXT, in link order and doubleword aligned.
; This one starts at 13E4h; its own empty segment 163 fixes its link position.
; Holds a shape's frame count.

	.MODEL  MEDIUM
	.386
	LOCALS

	PUBLIC  _GetShapeFrameCount

	EXTRN   _EnterFlatMode:FAR

; A shape starts with its size and a table of frame offsets; the first offset
; also marks where the table ends.
SHAPEHDR STRUC
shape_size          dd  ?
shape_firstFrame    dd  ?
SHAPEHDR ENDS

	.DATA
	EXTRN   _FlatModeFlags:WORD

LOWLEVEL_TEXT SEGMENT DWORD PUBLIC USE16 'CODE'
	ASSUME  cs:LOWLEVEL_TEXT

; Return how many frames a shape holds, from its first frame offset.
_GetShapeFrameCount PROC FAR
	ARG     shape:DWORD, flags:WORD
	enter   0, 0
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
	push    0
	push    eax
	pop     dx
	pop     eax
	shl     eax, 4
	add     eax, edx
@@flat:
	mov     esi, eax
	xor     eax, eax
	mov     ax, word ptr [esi].shape_firstFrame
	shr     ax, 1
	shr     ax, 1
	dec     ax
	pop     es
	pop     ds
	pop     esi
	leave
	ret
_GetShapeFrameCount ENDP

LOWLEVEL_TEXT ENDS

	END
