; Black Gate ENDGAME.EXE, resident segment 70 (file offsets 0x011d7e to 0x012094, 790 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

	.MODEL  MEDIUM, C
	LOCALS

	PUBLIC  DrawEmsFrameTranslated, DrawFrameTranslated

	EXTRN   MapEmsPointer:FAR

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

	.CODE

; DrawFrameTranslated for a shape that may sit in expanded memory: map it in first.
DrawEmsFrameTranslated PROC FAR view:WORD, x:WORD, y:WORD, shape:DWORD, frameNum:WORD, remap:WORD
	push    word ptr shape+2
	push    word ptr shape
	call    MapEmsPointer
	add     sp, 4
	or      dx, dx
	je      @@done
	mov     word ptr shape, ax
	mov     word ptr shape+2, dx
	push    remap
	push    frameNum
	push    word ptr shape+2
	push    word ptr shape
	push    y
	push    x
	push    view
	call    far ptr DrawFrameTranslated
	add     sp, 14
@@done:
	ret
DrawEmsFrameTranslated ENDP

; Recolor the screen under a shape frame through a 256-byte table, clipped to
; the view. Only the frame's outline matters; its pixel values are skipped.
DrawFrameTranslated PROC FAR view:WORD, x:WORD, y:WORD, shape:DWORD, frameNum:WORD, remap:WORD
	LOCAL   overhang:WORD, unused:WORD:2, clipBottom:WORD, clipRight:WORD, clipTop:WORD, \
		clipLeft:WORD, rowTable:WORD, rowSeg:WORD, spanBuf:BYTE:SCREEN_WIDTH, shapeSeg:WORD, \
		dataSeg:WORD
	USES    si, di, ds
	cld
	mov     dataSeg, ds                 ; where the table is
	push    ss                          ; take a copy of the view
	pop     es
	mov     si, view
	lea     di, rowSeg
	mov     cx, 6
	rep     movsw
	mov     es, rowSeg
	lds     si, shape
	mov     bx, frameNum
	inc     bx
	shl     bx, 1
	shl     bx, 1
	cmp     word ptr [si].shape_firstFrame, bx
	jb      @@noFrame
	jne     @@frameOk
@@noFrame:
	jmp     @@done
; find the frame, then test its bounds against the clip box
@@frameOk:
	mov     ax, ds                      ; the shape's linear address in dx:ax
	mov     dx, 0
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
	mov     shapeSeg, ax
	mov     ax, [si].frame_right
	add     ax, x
	cmp     ax, clipRight
	jg      @@testBottom                ; the jumps below still hold these flags
	mov     ax, x
	sub     ax, [si].frame_left
	cmp     ax, clipLeft
	jl      @@testTop
	mov     ax, y
	sub     ax, [si].frame_top
	cmp     ax, clipTop
@@testTop:
	jge     @@inside
	jmp     @@clipped
@@inside:
	mov     ax, [si].frame_bottom
	add     ax, y
	cmp     ax, clipBottom
@@testBottom:
	jle     @@fast
	jmp     @@clipped
; whole frame inside the clip box: no per-span checks
@@fast:
	mov     dx, ds
	add     si, SIZE FRAMEHDR
; span word: length*2 + 1 if run-encoded; 0 ends the frame
@@fastSpan:
	lodsw
	or      ax, ax
	je      @@spanEnd
	mov     cx, ax
	lodsw
	add     ax, x
	mov     di, ax
	lodsw
	add     ax, y
	shl     ax, 1
	mov     bx, rowTable
	add     bx, ax
	add     di, ss:[bx]
	mov     ds, dx
	shr     cx, 1
	jb      @@fastRle
	mov     ds, dataSeg
	mov     bx, remap
; skip the shape's pixel; recolor the screen pixel under it
@@fastPx:
	inc     si
	mov     al, es:[di]
	xlatb
	stosb
	loop    @@fastPx
	mov     ds, dx
	jmp     @@fastSpan
; run byte: count*2 + 1 if a fill
@@fastRle:
	mov     bx, cx
@@fastRun:
	lodsb
	shr     al, 1
	cbw
	mov     cx, ax
	jb      @@fastFill
	push    bx
	push    ax
	mov     ds, dataSeg
	mov     bx, remap
@@fastRunPx:
	inc     si
	mov     al, es:[di]
	xlatb
	stosb
	loop    @@fastRunPx
	mov     ds, dx
	pop     ax
	pop     bx
	sub     bx, ax
	jne     @@fastRun
	jmp     @@fastSpan
@@fastFill:
	sub     bx, ax
	lodsb
	push    bx
	mov     ds, dataSeg
	mov     bx, remap
@@fastFillPx:
	mov     al, es:[di]
	xlatb
	stosb
	loop    @@fastFillPx
	mov     ds, dx
	pop     bx
	or      bx, bx
	jne     @@fastRun
	jmp     @@fastSpan
@@done:
	ret
; frame crosses the clip box: clip every span
@@clipped:
	add     si, SIZE FRAMEHDR
@@clipSpan:
	lodsw
	shr     ax, 1
@@spanEnd:
	je      @@done
	mov     cx, ax
	jae     @@rawSpan
	jmp     @@rleSpan
@@rawSpan:
	lodsw
	add     ax, x
	mov     dx, ax
	lodsw
	add     ax, y
	cmp     ax, clipTop
	jl      @@skipRaw
	cmp     ax, clipBottom
	jg      @@skipRaw
	mov     bx, ax
	cmp     dx, clipRight
	jg      @@skipRaw
	mov     ax, dx
	add     ax, cx
	dec     ax
	cmp     ax, clipLeft
	jl      @@skipRaw
	shl     bx, 1
	push    ds
	mov     ax, rowTable
	add     bx, ax
	mov     di, ss:[bx]
	pop     ds
	add     di, dx
	mov     ax, clipLeft
	mov     bx, clipRight
	sub     ax, dx
	jle     @@rawRight
	sub     cx, ax
	add     si, ax
	add     di, ax
	add     dx, ax
@@rawRight:
	add     dx, cx
	dec     dx
	sub     dx, bx
	jle     @@rawAll
	sub     cx, dx
	mov     ds, dataSeg
	mov     bx, remap
@@rawPx:
	inc     si
	mov     al, es:[di]
	xlatb
	stosb
	loop    @@rawPx
	mov     ds, shapeSeg
	add     si, dx
	jmp     @@clipSpan
@@rawAll:
	mov     ds, dataSeg
	mov     bx, remap
@@rawAllPx:
	inc     si
	mov     al, es:[di]
	xlatb
	stosb
	loop    @@rawAllPx
	mov     ds, shapeSeg
	jmp     @@clipSpan
@@skipRaw:
	add     si, cx
	jmp     @@clipSpan
; run-encoded span
@@rleSpan:
	lodsw
	add     ax, x
	mov     dx, ax
	lodsw
	add     ax, y
	cmp     ax, clipTop
	jl      @@skipRle
	cmp     ax, clipBottom
	jg      @@skipRle
	mov     bx, ax
	cmp     dx, clipRight
	jg      @@skipRle
	mov     ax, dx
	add     ax, cx
	dec     ax
	cmp     ax, clipLeft
	jl      @@skipRle
	shl     bx, 1
	push    ds
	mov     di, rowTable
	add     bx, di
	mov     di, ss:[bx]
	pop     ds
	add     di, dx
	sub     ax, clipRight
	sub     dx, clipLeft
	jl      @@rleBuffer
	cmp     ax, 0
	jg      @@rleBuffer
	mov     bx, cx
@@rleRun:
	lodsb
	shr     al, 1
	cbw
	mov     cx, ax
	jb      @@rleFill
	push    bx
	push    ax
	mov     ds, dataSeg
	mov     bx, remap
@@rleRunPx:
	inc     si
	mov     al, es:[di]
	xlatb
	stosb
	loop    @@rleRunPx
	mov     ds, shapeSeg
	pop     ax
	pop     bx
	sub     bx, ax
	jne     @@rleRun
	jmp     @@clipSpan
@@rleFill:
	sub     bx, ax
	lodsb
	mov     ah, al
	push    bx
	mov     ds, dataSeg
	mov     bx, remap
@@rleFillPx:
	mov     al, es:[di]
	xlatb
	stosb
	loop    @@rleFillPx
	mov     ds, shapeSeg
	pop     bx
	or      bx, bx
	jne     @@rleRun
	jmp     @@clipSpan
; span fully clipped: step over its runs
@@skipRle:
	lodsb
	add     si, 1
	shr     al, 1
	cbw
	jb      @@skipNext
	add     si, ax
	dec     si
@@skipNext:
	sub     cx, ax
	jne     @@skipRle
	jmp     @@clipSpan
; partly clipped runs: go through spanBuf, then do the visible part
@@rleBuffer:
	mov     overhang, ax
	mov     bx, cx
	push    cx
	push    es
	push    di
	push    ss
	pop     es
	lea     di, spanBuf
@@bufRun:
	lodsb
	shr     al, 1
	cbw
	mov     cx, ax
	jb      @@bufFill
	push    bx
	push    ax
	mov     ds, dataSeg
	mov     bx, remap
@@bufRunPx:
	lodsb
	mov     al, es:[di]
	xlatb
	stosb
	loop    @@bufRunPx
	mov     ds, shapeSeg
	pop     ax
	pop     bx
	sub     bx, ax
	jne     @@bufRun
	jmp     short @@bufCopy
@@bufFill:
	sub     bx, ax
	lodsb
	mov     ah, al
	push    bx
	mov     ds, dataSeg
	mov     bx, remap
@@bufFillPx:
	mov     al, es:[di]
	xlatb
	stosb
	loop    @@bufFillPx
	mov     ds, shapeSeg
	pop     bx
	or      bx, bx
	jne     @@bufRun
@@bufCopy:
	pop     di
	pop     es
	pop     cx
	push    ds
	push    si
	push    ss
	pop     ds
	lea     si, spanBuf
	mov     ax, 0
	cmp     dx, 0
	jge     @@bufRight
	add     cx, dx
	sub     si, dx
	sub     di, dx
@@bufRight:
	cmp     overhang, 0
	jle     @@bufOut
	sub     cx, overhang
	mov     ax, overhang
@@bufOut:
	push    bx
	push    ax
	mov     ds, dataSeg
	mov     bx, remap
@@bufOutPx:
	lodsb
	mov     al, es:[di]
	xlatb
	stosb
	loop    @@bufOutPx
	mov     ds, shapeSeg
	pop     ax
	pop     bx
	add     si, ax
	pop     si
	pop     ds
	jmp     @@clipSpan
DrawFrameTranslated ENDP

	END
