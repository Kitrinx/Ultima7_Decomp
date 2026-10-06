; Serpent Isle MAINMENU.EXE, resident segment 59 (file offsets 0x0189dc to 0x018f8d, 1457 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

; Holds drawing a translucent frame.

	.MODEL  MEDIUM
	.386
	LOCALS

	PUBLIC  _DrawFrameTranslucent

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

; Draw a shape frame clipped to the view, blending each pixel with the screen
; through a translucency table. Pixels whose first-table entry is 0FFh are
; drawn solid.
; Flags 1 and 10h: the shape and the table addresses are linear.
_DrawFrameTranslucent PROC FAR
	ARG     view:WORD, x:WORD, y:WORD, shape:DWORD, frameNum:WORD, blend:DWORD, flags:WORD
	LOCAL   overhang:WORD, unused:WORD:2, clipBottom:WORD, clipRight:WORD, clipTop:WORD, \
		clipLeft:WORD, rowPtr:DWORD, rowSeg:WORD, blendAt:DWORD, spanBuf:BYTE:SCREEN_WIDTH = frame
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
	mov     eax, blend
	jne     short @@blendLinear
	xor     edx, edx
	push    0
	push    eax
	pop     dx
	pop     eax
	shl     eax, 4
	add     eax, edx
@@blendLinear:
	mov     blendAt, eax
	xor     eax, eax
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
; decode the frame's span table entry
@@frameOk:
	mov     ecx, esi
	add     esi, ebx
	mov     eax, [esi]
	mov     esi, ecx
	add     esi, eax
	mov     ax, [esi].frame_right
	add     ax, x
	cmp     ax, clipRight
	jg      @@toClipped
	mov     ax, x
	sub     ax, [esi].frame_left
	cmp     ax, clipLeft
	jl      @@toClipped
	mov     ax, y
	sub     ax, [esi].frame_top
	cmp     ax, clipTop
	jl      @@toClipped
	mov     ax, [esi].frame_bottom
	add     ax, y
	cmp     ax, clipBottom
	jle     @@fast
@@toClipped:
	jmp     @@clipped
; whole frame inside the clip: no per-span checks
@@fast:
	add     esi, SIZE FRAMEHDR
; span word: length*2 + 1 if run-encoded; 0 ends the frame
@@fastSpan:
	mov     edi, edx
	lods    word ptr [esi]
	or      ax, ax
	jne     @@fastDraw
	jmp     @@done
@@fastDraw:
	movzx   ecx, ax
	xor     eax, eax
	lods    word ptr [esi]
	add     ax, x
	add     edi, eax
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
	mov     es, ax
	mov     ds, ax
	shr     cx, 1
	jb      @@fastRle
	push    eax
	push    ebx
	push    edx
	mov     ebx, blendAt
	xor     eax, eax
; 0FFh in the first table keeps the pixel solid; any other entry picks
; the 256-byte table the screen pixel is looked up in
@@fastPx:
	lods    byte ptr [esi]
	push    eax
	xlat    byte ptr [ebx]
	cmp     al, 0FFh
	jne     @@fastMix
	pop     eax
	jmp     @@fastPut
@@fastMix:
	shl     eax, 8
	push    ebx
	add     ebx, eax
	xor     eax, eax
	mov     al, [edi]
	xlat    byte ptr [ebx]
	pop     ebx
	pop     edx
@@fastPut:
	stos    byte ptr es:[edi]
	xor     eax, eax
	loop    @@fastPx
	pop     edx
	pop     ebx
	pop     eax
	jmp     @@fastSpan
@@fastRle:
	mov     bx, cx
; run byte: count*2 + 1 if a fill
@@fastRun:
	xor     eax, eax
	lods    byte ptr [esi]
	shr     al, 1
	cbw
	mov     cx, ax
	jb      @@fastFill
	push    eax
	push    ebx
	push    edx
	mov     ebx, blendAt
	xor     eax, eax
@@fastRunPx:
	lods    byte ptr [esi]
	push    eax
	xlat    byte ptr [ebx]
	cmp     al, 0FFh
	jne     @@fastRunMix
	pop     eax
	jmp     @@fastRunPut
@@fastRunMix:
	shl     eax, 8
	push    ebx
	add     ebx, eax
	xor     eax, eax
	mov     al, [edi]
	xlat    byte ptr [ebx]
	pop     ebx
	pop     edx
@@fastRunPut:
	stos    byte ptr es:[edi]
	xor     eax, eax
	loop    @@fastRunPx
	pop     edx
	pop     ebx
	pop     eax
	sub     bx, ax
	jne     @@fastRun
	jmp     @@fastSpan
@@fastFill:
	sub     bx, ax
	push    eax
	push    ebx
	push    edx
	mov     ebx, blendAt
	xor     eax, eax
	lods    byte ptr [esi]
	push    eax
	xlat    byte ptr [ebx]
	cmp     al, 0FFh
	jne     @@fastFillMix
	pop     eax
	rep     stos byte ptr es:[edi]
	jmp     @@fastFillNext
@@fastFillMix:
	shl     eax, 8
	push    ebx
	add     ebx, eax
	xor     eax, eax
@@fastFillPx:
	mov     al, [edi]
	xlat    byte ptr [ebx]
	stos    byte ptr es:[edi]
	xor     eax, eax
	loop    @@fastFillPx
	pop     ebx
	pop     edx
@@fastFillNext:
	pop     edx
	pop     ebx
	pop     eax
	or      bx, bx
	jne     @@fastRun
	jmp     @@fastSpan
; clip each span against the view
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
	add     ax, x
	movsx   edx, ax
	lods    word ptr [esi]
	add     ax, y
	cmp     ax, clipTop
	jl      @@toSkipRaw
	cmp     ax, clipBottom
	jg      @@toSkipRaw
	movsx   ebx, ax
	cmp     dx, clipRight
	jg      @@toSkipRaw
	mov     eax, edx
	add     eax, ecx
	dec     eax
	cmp     ax, clipLeft
	jge     @@rawVisible
@@toSkipRaw:
	jmp     @@skipRaw
@@rawVisible:
	shl     bx, 2
	mov     eax, rowPtr
	add     ebx, eax
	xor     eax, eax
	mov     eax, [ebx]
	add     edi, eax
	add     edi, edx
	movzx   eax, word ptr clipLeft
	movzx   ebx, word ptr clipRight
	sub     eax, edx
	jle     @@rawRight
	sub     ecx, eax
	add     esi, eax
	add     edi, eax
	add     edx, eax
@@rawRight:
	add     edx, ecx
	dec     edx
	sub     edx, ebx
	jle     @@rawCopy
	sub     ecx, edx
	push    eax
	push    ebx
	push    edx
	mov     ebx, blendAt
	xor     eax, eax
@@rawPx:
	lods    byte ptr [esi]
	push    eax
	xlat    byte ptr [ebx]
	cmp     al, 0FFh
	jne     @@rawMix
	pop     eax
	jmp     @@rawPut
@@rawMix:
	shl     eax, 8
	push    ebx
	add     ebx, eax
	xor     eax, eax
	mov     al, [edi]
	xlat    byte ptr [ebx]
	pop     ebx
	pop     edx
@@rawPut:
	stos    byte ptr es:[edi]
	xor     eax, eax
	loop    @@rawPx
	pop     edx
	pop     ebx
	pop     eax
	add     esi, edx
	jmp     @@clipSpan
@@rawCopy:
	push    eax
	push    ebx
	push    edx
	mov     ebx, blendAt
	xor     eax, eax
@@copyPx:
	lods    byte ptr [esi]
	push    eax
	xlat    byte ptr [ebx]
	cmp     al, 0FFh
	jne     @@copyMix
	pop     eax
	jmp     @@copyPut
@@copyMix:
	shl     eax, 8
	push    ebx
	add     ebx, eax
	xor     eax, eax
	mov     al, [edi]
	xlat    byte ptr [ebx]
	pop     ebx
	pop     edx
@@copyPut:
	stos    byte ptr es:[edi]
	xor     eax, eax
	loop    @@copyPx
	pop     edx
	pop     ebx
	pop     eax
	jmp     @@clipSpan
@@skipRaw:
	add     esi, ecx
	jmp     @@clipSpan
@@rleSpan:
	lods    word ptr [esi]
	add     ax, x
	movsx   edx, ax
	lods    word ptr [esi]
	add     ax, y
	cmp     ax, clipTop
	jl      @@toSkipRle
	cmp     ax, clipBottom
	jg      @@toSkipRle
	movsx   ebx, ax
	cmp     dx, clipRight
	jg      @@toSkipRle
	mov     eax, edx
	add     eax, ecx
	dec     eax
	cmp     ax, clipLeft
	jge     @@rleVisible
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
	add     edi, edx
	pop     eax
	xor     ebx, ebx
	movsx   ebx, word ptr clipRight
	sub     eax, ebx
	movsx   ebx, word ptr clipLeft
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
	push    edx
	mov     ebx, blendAt
	xor     eax, eax
@@rleRunPx:
	lods    byte ptr [esi]
	push    eax
	xlat    byte ptr [ebx]
	cmp     al, 0FFh
	jne     @@rleRunMix
	pop     eax
	jmp     @@rleRunPut
@@rleRunMix:
	shl     eax, 8
	push    ebx
	add     ebx, eax
	xor     eax, eax
	mov     al, [edi]
	xlat    byte ptr [ebx]
	pop     ebx
	pop     edx
@@rleRunPut:
	stos    byte ptr es:[edi]
	xor     eax, eax
	loop    @@rleRunPx
	pop     edx
	pop     ebx
	pop     eax
	sub     bx, ax
	jne     @@rleRun
	jmp     @@clipSpan
@@rleFill:
	sub     ebx, eax
	push    eax
	push    ebx
	push    edx
	mov     ebx, blendAt
	xor     eax, eax
	lods    byte ptr [esi]
	push    eax
	xlat    byte ptr [ebx]
	cmp     al, 0FFh
	jne     @@rleFillMix
	pop     eax
	rep     stos byte ptr es:[edi]
	jmp     @@rleFillNext
@@rleFillMix:
	shl     eax, 8
	push    ebx
	add     ebx, eax
	xor     eax, eax
@@rleFillPx:
	mov     al, [edi]
	xlat    byte ptr [ebx]
	stos    byte ptr es:[edi]
	xor     eax, eax
	loop    @@rleFillPx
	pop     ebx
	pop     edx
@@rleFillNext:
	pop     edx
	pop     ebx
	pop     eax
	or      ebx, ebx
	jne     @@rleRun
	jmp     @@clipSpan
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
; partly clipped run: decode into spanBuf, then copy the visible part
@@rleBuffer:
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
	mov     ax, ss
	shl     eax, 4
	lea     si, spanBuf
	add     esi, eax
	cmp     edx, 0
	jge     @@bufRight
	xor     edx, -1
	inc     edx
	add     esi, edx
	sub     ecx, edx
	add     edi, edx
@@bufRight:
	xor     eax, eax
	mov     ds, ax
	cmp     word ptr overhang, 0
	jle     @@bufOut
	sub     cx, overhang
	movzx   eax, word ptr overhang
@@bufOut:
	push    eax
	push    ebx
	push    edx
	mov     ebx, blendAt
	xor     eax, eax
@@bufPx:
	lods    byte ptr [esi]
	push    eax
	xlat    byte ptr [ebx]
	cmp     al, 0FFh
	jne     @@bufMix
	pop     eax
	jmp     @@bufPut
@@bufMix:
	shl     eax, 8
	push    ebx
	add     ebx, eax
	xor     eax, eax
	mov     al, [edi]
	xlat    byte ptr [ebx]
	pop     ebx
	pop     edx
@@bufPut:
	stos    byte ptr es:[edi]
	xor     eax, eax
	loop    @@bufPx
	pop     edx
	pop     ebx
	pop     eax
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
_DrawFrameTranslucent ENDP

LOWLEVEL_TEXT ENDS

	END
