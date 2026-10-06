; Serpent Isle SI.EXE, one module of resident segment 58 (file offsets 0x024f34 to 0x02548f, 1371 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

; Holds drawing a mirrored frame through a color table.

	.MODEL  MEDIUM
	.386
	LOCALS

	PUBLIC  _DrawFrameFlippedTranslated

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

; Shade the pixels under a frame, x and y swapped: each covered screen
; pixel is replaced by its entry in the remap table. Clipped like _DrawFrame.
; Flags 1 and 10h: the shape and the table addresses are linear.
_DrawFrameFlippedTranslated PROC FAR
	ARG     view:WORD, x:WORD, y:WORD, shape:DWORD, frameNum:WORD, remap:DWORD, flags:WORD
	LOCAL   overhang:WORD, unused:WORD:2, clipBottom:WORD, clipRight:WORD, clipTop:WORD, \
		clipLeft:WORD, rowPtr:DWORD, rowSeg:WORD, remapAt:DWORD, spanBuf:BYTE:SCREEN_WIDTH = frame
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
	mov     ax, flags
	and     ax, 10h
	mov     eax, remap
	jne     short @@remapLinear
	xor     edx, edx
	push    0
	push    eax
	pop     dx
	pop     eax
	shl     eax, 4
	add     eax, edx
; remap table as a linear address
@@remapLinear:
	mov     remapAt, eax
	xor     eax, eax
	xor     eax, eax
	mov     edx, eax
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
	xor     ebx, ebx
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
	push    eax
	push    ebx
	xor     eax, eax
	mov     ebx, remapAt
@@fastPx:
	inc     esi
	mov     al, [edi]
	xlat    byte ptr [ebx]
	stos    byte ptr es:[edi]
	add     edi, SCREEN_WIDTH - 1
	loop    @@fastPx
	pop     ebx
	pop     eax
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
	push    eax
	push    ebx
	xor     eax, eax
	mov     ebx, remapAt
@@fastRunPx:
	inc     esi
	mov     al, [edi]
	xlat    byte ptr [ebx]
	stos    byte ptr es:[edi]
	add     edi, SCREEN_WIDTH - 1
	loop    @@fastRunPx
	pop     ebx
	pop     eax
	sub     ebx, eax
	jne     @@fastRun
	jmp     @@fastSpan
@@fastFill:
	sub     ebx, eax
	xor     eax, eax
	inc     si
	push    eax
	push    ebx
	xor     eax, eax
	mov     ebx, remapAt
@@fastFillPx:
	mov     al, [edi]
	xlat    byte ptr [ebx]
	stos    byte ptr es:[edi]
	add     edi, SCREEN_WIDTH - 1
	loop    @@fastFillPx
	pop     ebx
	pop     eax
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
	cmp     ebx, 0
	jge     @@rawVisible
	mov     ebx, 0
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
	cmp     edx, 0
	jle     @@rawColumn
	add     edi, edx
@@rawColumn:
	mov     edx, ebx
	movzx   eax, word ptr clipTop
	cmp     edx, eax
	jge     @@rawLead
	xor     ebx, ebx
	mov     bx, clipTop
	cmp     ebx, 0
	je      @@rawLead
	cmp     edx, 0
	jle     @@rawDown
	sub     ebx, edx
@@rawDown:
	add     edi, SCREEN_WIDTH
	dec     ebx
	jne     @@rawDown
@@rawLead:
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
	push    eax
	push    ebx
	xor     eax, eax
	mov     ebx, remapAt
@@rawPx:
	inc     esi
	mov     al, [edi]
	xlat    byte ptr [ebx]
	stos    byte ptr es:[edi]
	add     edi, SCREEN_WIDTH - 1
	loop    @@rawPx
	pop     ebx
	pop     eax
@@rawSkipTail:
	add     esi, edx
	jmp     @@clipSpan
@@rawCopy:
	or      ecx, ecx
	je      @@rawDone
	push    eax
	push    ebx
	xor     eax, eax
	mov     ebx, remapAt
@@copyPx:
	inc     esi
	mov     al, [edi]
	xlat    byte ptr [ebx]
	stos    byte ptr es:[edi]
	add     edi, SCREEN_WIDTH - 1
	loop    @@copyPx
	pop     ebx
	pop     eax
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
	cmp     bx, 0
	jge     @@rleVisible
	mov     ebx, 0
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
	xor     ebx, ebx
	movsx   ebx, word ptr clipBottom
	sub     eax, ebx
	movsx   ebx, word ptr clipTop
	sub     edx, ebx
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
	push    eax
	push    ebx
	xor     eax, eax
	mov     ebx, remapAt
@@rleRunPx:
	inc     esi
	mov     al, [edi]
	xlat    byte ptr [ebx]
	stos    byte ptr es:[edi]
	add     edi, SCREEN_WIDTH - 1
	loop    @@rleRunPx
	pop     ebx
	pop     eax
	sub     ebx, eax
	jne     @@rleRun
	jmp     @@clipSpan
@@rleFill:
	sub     ebx, eax
	inc     esi
	push    eax
	push    ebx
	xor     eax, eax
	mov     ebx, remapAt
@@rleFillPx:
	mov     al, [edi]
	xlat    byte ptr [ebx]
	stos    byte ptr es:[edi]
	add     edi, SCREEN_WIDTH - 1
	loop    @@rleFillPx
	pop     ebx
	pop     eax
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
	push    ebx
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
	pop     ebx
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
	push    edx
	cmp     ebx, 0
	jle     @@bufTrim
	cmp     edx, ebx
	jle     @@bufClamp
	jmp     @@bufDown
@@bufClamp:
	mov     ebx, edx
@@bufDown:
	add     edi, SCREEN_WIDTH
	dec     ebx
	jne     @@bufDown
@@bufTrim:
	pop     edx
	sub     ecx, edx
@@bufLead:
	inc     esi
	dec     edx
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
	push    eax
	push    ebx
	xor     eax, eax
	mov     ebx, remapAt
@@bufPx:
	inc     esi
	mov     al, [edi]
	xlat    byte ptr [ebx]
	stos    byte ptr es:[edi]
	add     edi, SCREEN_WIDTH - 1
	loop    @@bufPx
	pop     ebx
	pop     eax
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
_DrawFrameFlippedTranslated ENDP

LOWLEVEL_TEXT ENDS

	END
