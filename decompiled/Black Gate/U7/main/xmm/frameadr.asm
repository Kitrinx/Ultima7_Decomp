; Black Gate U7.EXE, one module of resident segment 27 (file offsets 0x015ff0 to 0x016082, 146 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.
; Segment 27 joins 25 modules' code as LOWLEVEL_TEXT, in link order and doubleword aligned.
; This one starts at 129Ch; its own empty segment 155 fixes its link position.
; Holds finding a shape frame.

	.MODEL  MEDIUM
	.386
	LOCALS

	PUBLIC  _GetFrameAddress

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

; Return the address of one frame of a shape, or 0 if there is none.
; Flag 1: the shape is flat. Flag 100h: return flat, else segment:offset.
_GetFrameAddress PROC FAR
	ARG     shape:DWORD, frameNum:WORD, flags:WORD
	enter   0, 0
	push    ds
	push    es
	push    esi
	push    edi
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
	xor     ebx, ebx
	mov     bx, frameNum
	inc     bx
	shl     bx, 1
	shl     bx, 1
	cmp     bx, word ptr [esi].shape_firstFrame
	jle     @@inRange
	xor     eax, eax
	jmp     short @@convert
@@inRange:
	mov     eax, [esi+ebx]
	or      eax, eax
	jne     @@found
	xor     eax, eax
	jmp     @@convert
@@found:
	add     eax, esi
@@convert:
	xor     ebx, ebx
	mov     bx, flags
	and     bh, 1
	jne     short @@done
	mov     bx, ax
	shr     eax, 4
	push    ax
	and     bx, 0FFh
	push    bx
	pop     eax
@@done:
	push    eax
	pop     ax
	pop     dx
	pop     edi
	pop     esi
	pop     es
	pop     ds
	leave
	ret
_GetFrameAddress ENDP

LOWLEVEL_TEXT ENDS

	END
