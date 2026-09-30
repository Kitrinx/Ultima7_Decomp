; Black Gate U7.EXE, one module of resident segment 27 (file offsets 0x016ccc to 0x01711b, 1103 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.
; Segment 27 joins 25 modules' code as LOWLEVEL_TEXT, in link order and doubleword aligned.
; This one starts at 1F78h; its own empty segment 171 fixes its link position.
; Holds drawing a mirrored shape frame.

	.MODEL  MEDIUM
	.386
	LOCALS

	PUBLIC  _DrawFrameFlipped

	EXTRN   _EnterFlatMode:FAR

SCREEN_WIDTH    EQU 320

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

; Draw one frame of a run-length shape with x and y swapped, so each
; span runs down a column. Clipped to the view's clip box like _DrawFrame.
; Flag 1: the shape address is linear, else segment:offset.
_DrawFrameFlipped PROC FAR
	ARG     view:WORD, x:WORD, y:WORD, shape:DWORD, frameNum:WORD, flags:WORD
	LOCAL   overhang:WORD, unused:WORD:2, clipBottom:WORD, clipRight:WORD, clipTop:WORD, \
		clipLeft:WORD, rowPtr:DWORD, rowSeg:WORD, spanBuf:BYTE:SCREEN_WIDTH = frame
	enter   frame, 0
	push    edi
	push    esi
	push    ds
	push    es
	push    fs
	push    gs
	pushf
	cld
	mov     ax, word ptr _FlatModeFlags
	and     ax, 1
	je      @@ready
	call    far ptr _EnterFlatMode
@@ready:
	push    ds
	pop     fs
	push    ss
	pop     es
	mov     si, view
	lea     di, rowSeg
	movsd
	movsd
	movsd
	movsw
	xor     eax, eax
	mov     ax, flags
	and     ax, 1
	mov     eax, shape
	jne     short @@linear
	xor     edx, edx
	push    0
	push    eax
	pop     dx
	pop     eax
	shl     eax, 4
	add     eax, edx
@@linear:
	mov     esi, eax
	xor     eax, eax
	mov     ds, ax
	xor     eax, eax
	mov     ax, rowSeg
	shl     eax, 4
	mov     edx, eax
	mov     ax, flags
	and     eax, 100h
	je      short @@haveBase
	shl     edx, 12
@@haveBase:
	mov     edi, edx
	xor     eax, eax
	mov     es, ax
	xor     ebx, ebx
	mov     bx, frameNum
	inc     bx
	shl     bx, 1
	shl     bx, 1
	cmp     word ptr [esi].shape_firstFrame, bx
	jae     @@frameOk
	jmp     @@done
; find the frame, then test its bounds against the clip box
@@frameOk:
	mov     ecx, esi
	add     esi, ebx
	mov     eax, [esi]
	mov     esi, ecx
	add     esi, eax
	mov     ax, [esi].frame_bottom
	add     ax, x
	cmp     ax, clipRight
	jg      @@toClipped
	mov     ax, x
	sub     ax, [esi].frame_top
	cmp     ax, clipLeft
	jl      @@toClipped
	mov     ax, y
	sub     ax, [esi].frame_left
	cmp     ax, clipTop
	jl      @@toClipped
	mov     ax, [esi].frame_right
	add     ax, y
	cmp     ax, clipBottom
	jle     @@fast
@@toClipped:
	jmp     @@clipped
; whole frame inside the clip box: no per-span checks
@@fast:
	add     esi, SIZE FRAMEHDR
; span word: length*2 + 1 if run-encoded; 0 ends the frame
@@fastSpan:
	mov     edi, edx
	xor     eax, eax
	lods    word ptr [esi]
	or      ax, ax
	jne     @@fastDraw
	jmp     @@done
@@fastDraw:
	mov     ecx, eax
	xor     eax, eax
	lods    word ptr [esi]
	add     ax, y
	shl     eax, 2
	mov     ebx, rowPtr
	add     ebx, eax
	xor     eax, eax
	mov     eax, [ebx]
	add     edi, eax
	xor     eax, eax
	lods    word ptr [esi]
	add     ax, x
	add     edi, eax
	xor     eax, eax
	mov     es, ax
	mov     ds, ax
	shr     ecx, 1
	jb      @@fastRle
@@fastPx:
	movs    byte ptr es:[edi], byte ptr [esi]
	add     edi, SCREEN_WIDTH - 1
	loop    @@fastPx
	jmp     @@fastSpan
; run byte: count*2 + 1 if a fill of one color
@@fastRle:
	mov     ebx, ecx
@@fastRun:
	xor     eax, eax
	lods    byte ptr [esi]
	shr     al, 1
	cbw
	movzx   ecx, ax
	jb      @@fastFill
@@fastRunPx:
	movs    byte ptr es:[edi], byte ptr [esi]
	add     edi, SCREEN_WIDTH - 1
	loop    @@fastRunPx
	sub     ebx, eax
	jne     @@fastRun
	jmp     @@fastSpan
@@fastFill:
	sub     ebx, eax
	xor     eax, eax
	lods    byte ptr [esi]
@@fastFillPx:
	stos    byte ptr es:[edi]
	add     edi, SCREEN_WIDTH - 1
	loop    @@fastFillPx
	or      ebx, ebx
	jne     @@fastRun
	jmp     @@fastSpan
; frame crosses the clip box: clip every span
@@clipped:
	add     esi, SIZE FRAMEHDR
@@clipSpan:
	xor     eax, eax
	lods    word ptr [esi]
	shr     eax, 1
	jne     @@clipDraw
	jmp     @@done
@@clipDraw:
	mov     edi, 0
	mov     ecx, eax
	jb      @@rleSpan
	xor     eax, eax
	lods    word ptr [esi]
	add     ax, y
	movsx   ebx, ax
	lods    word ptr [esi]
	add     ax, x
	movsx   edx, ax
	mov     eax, ebx
	add     eax, ecx
	cmp     ax, clipTop
	mov     eax, ebx
	jl      @@toSkipRaw
	cmp     ax, clipBottom
	jg      @@toSkipRaw
	cmp     dx, clipRight
	jg      @@toSkipRaw
	mov     eax, edx
	cmp     ax, clipLeft
	jl      @@toSkipRaw
	push    ebx
	cmp     bx, clipTop
	jge     @@rawVisible
	movzx   ebx, word ptr clipTop
	jmp     @@rawVisible
@@toSkipRaw:
	jmp     @@skipRaw
@@rawVisible:
	shl     bx, 2
	mov     eax, rowPtr
	add     ebx, eax
	xor     eax, eax
	mov     eax, [ebx]
	add     edi, eax
	pop     ebx
	add     edi, edx
	mov     edx, ebx
	movzx   eax, word ptr clipTop
	sub     eax, edx
	jle     @@rawRight
	mov     ebx, ecx
	sub     ecx, eax
	add     edx, eax
	sub     ebx, ecx
@@skipLead:
	inc     esi
	dec     bx
	jne     @@skipLead
@@rawRight:
	movzx   ebx, word ptr clipBottom
	add     edx, ecx
	dec     edx
	sub     edx, ebx
	jle     @@rawCopy
	sub     ecx, edx
	or      ecx, ecx
	je      @@rawSkipTail
@@rawPx:
	movs    byte ptr es:[edi], byte ptr [esi]
	add     edi, SCREEN_WIDTH - 1
	dec     cx
	jne     @@rawPx
@@rawSkipTail:
	add     esi, edx
	jmp     @@clipSpan
@@rawCopy:
	or      ecx, ecx
	je      @@rawDone
@@copyPx:
	movs    byte ptr es:[edi], byte ptr [esi]
	add     edi, SCREEN_WIDTH - 1
	dec     cx
	jne     @@copyPx
@@rawDone:
	jmp     @@clipSpan
@@skipRaw:
	add     esi, ecx
	jmp     @@clipSpan
; run-encoded span
@@rleSpan:
	lods    word ptr [esi]
	add     ax, y
	movsx   ebx, ax
	lods    word ptr [esi]
	add     ax, x
	mov     edx, eax
	mov     eax, ebx
	add     eax, ecx
	cmp     ax, clipTop
	mov     eax, ebx
	jl      @@toSkipRle
	cmp     ax, clipBottom
	jg      @@toSkipRle
	cmp     dx, clipRight
	jg      @@toSkipRle
	mov     eax, edx
	cmp     ax, clipLeft
	jl      @@toSkipRle
	push    ebx
	cmp     bx, clipTop
	jge     @@rleVisible
	movzx   ebx, word ptr clipTop
	jmp     @@rleVisible
@@toSkipRle:
	jmp     @@skipRle
@@rleVisible:
	shl     ebx, 2
	push    eax
	mov     eax, rowPtr
	add     ebx, eax
	xor     eax, eax
	mov     es, ax
	mov     eax, [ebx]
	add     edi, eax
	cmp     edx, 0
	jle     @@rleEdges
	add     edi, edx
@@rleEdges:
	pop     eax
	pop     ebx
	mov     eax, ebx
	mov     edx, eax
	add     eax, ecx
	push    ebx
	xor     ebx, ebx
	movsx   ebx, word ptr clipBottom
	sub     eax, ebx
	movsx   ebx, word ptr clipTop
	sub     edx, ebx
	pop     ebx
	jge     @@rleRight
	jmp     @@rleBuffer
@@rleRight:
	cmp     eax, 0
	jle     @@rleWhole
	jmp     @@rleBuffer
@@rleWhole:
	movzx   ebx, cx
@@rleRun:
	xor     eax, eax
	lods    byte ptr [esi]
	shr     al, 1
	cbw
	movzx   ecx, ax
	jb      @@rleFill
@@rleRunPx:
	movs    byte ptr es:[edi], byte ptr [esi]
	add     edi, SCREEN_WIDTH - 1
	loop    @@rleRunPx
	sub     ebx, eax
	jne     @@rleRun
	jmp     @@clipSpan
@@rleFill:
	sub     ebx, eax
	lods    byte ptr [esi]
@@rleFillPx:
	stos    byte ptr es:[edi]
	add     edi, SCREEN_WIDTH - 1
	loop    @@rleFillPx
	or      ebx, ebx
	jne     @@rleRun
	jmp     @@clipSpan
; span fully clipped: step over its runs
@@skipRle:
	xor     eax, eax
	lods    byte ptr [esi]
	add     esi, 1
	shr     al, 1
	cbw
	jb      @@skipNext
	add     esi, eax
	dec     esi
@@skipNext:
	sub     ecx, eax
	jne     @@skipRle
	jmp     @@clipSpan
; partly clipped runs: decode into spanBuf, then copy the visible part
@@rleBuffer:
	dec     ax
	mov     overhang, ax
	movsx   ebx, cx
	push    cx
	push    es
	push    edi
	xor     edi, edi
	mov     es, di
	xor     eax, eax
	mov     ax, ss
	lea     di, spanBuf
	shl     eax, 4
	add     edi, eax
@@bufRun:
	xor     eax, eax
	lods    byte ptr [esi]
	shr     al, 1
	cbw
	movzx   ecx, ax
	jb      @@bufFill
	push    ecx
	and     ecx, 3
	rep     movs byte ptr es:[edi], byte ptr [esi]
	pop     ecx
	shr     ecx, 2
	rep     movs dword ptr es:[edi], dword ptr [esi]
	sub     ebx, eax
	jne     @@bufRun
	jmp     short @@bufCopy
@@bufFill:
	sub     ebx, eax
	lods    byte ptr [esi]
	mov     ah, al
	push    ax
	push    ax
	pop     eax
	push    ecx
	and     ecx, 3
	rep     stos byte ptr es:[edi]
	pop     ecx
	shr     ecx, 2
	rep     stos dword ptr es:[edi]
	or      bx, bx
	jne     @@bufRun
@@bufCopy:
	pop     edi
	pop     es
	pop     cx
	push    ds
	push    esi
	xor     esi, esi
	xor     eax, eax
	mov     ds, ax
	mov     ax, ss
	shl     eax, 4
	lea     si, spanBuf
	add     esi, eax
	xor     eax, eax
	cmp     edx, 0
	jge     @@bufRight
	xor     edx, -1
	inc     edx
	mov     eax, ecx
	sub     ecx, edx
	sub     eax, ecx
@@bufLead:
	inc     esi
	dec     eax
	jne     @@bufLead
@@bufRight:
	xor     eax, eax
	mov     ds, ax
	cmp     word ptr overhang, 0
	jle     @@bufOut
	sub     cx, overhang
	movzx   eax, word ptr overhang
@@bufOut:
	jcxz    @@bufDone
@@bufPx:
	movs    byte ptr es:[edi], byte ptr [esi]
	add     edi, SCREEN_WIDTH - 1
	loop    @@bufPx
@@bufDone:
	pop     esi
	pop     ds
	jmp     @@clipSpan
@@done:
	xor     eax, eax
	popf
	pop     gs
	pop     fs
	pop     es
	pop     ds
	pop     esi
	pop     edi
	leave
	ret
_DrawFrameFlipped ENDP

LOWLEVEL_TEXT ENDS

	END
