; Black Gate INTRO.EXE, resident segment 79 (file offsets 0x019ad2 to 0x01a274, 1954 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

	.MODEL  MEDIUM
	LOCALS

	INCLUDE scalfram.inc

	.CODE

; Draw a shape frame enlarged (scale 100h and up) and turned a quarter, one
; routine per flip. Frame rows become screen columns and each span a column of
; pixels. bx is the scale in 256ths throughout.
EnlargeSidewaysMirror PROC FAR
	call    far ptr PlaceEnlargedSidewaysMirror
	jb      @@offView
	mov     ax, ss:DrawRight
	mov     ss:ScreenX, ax
	mov     ax, ss:DrawBottom
	mov     ss:ScreenY, ax
	call    far ptr FrameNeedsClip
	jae     @@inside
	mov     ss:SpanProc, offset ClipColumnUp
	jmp     ClippedColumnsLeftward
@@offView:
	DRAW_EXIT
@@inside:
	mov     ss:SpanProc, offset ColumnUp
	jmp     ColumnsLeftward
EnlargeSidewaysMirror ENDP

EnlargeSideways PROC FAR
	call    far ptr PlaceEnlargedSideways
	jb      @@offView
	mov     ax, ss:DrawRight
	mov     ss:ScreenX, ax
	call    far ptr FrameNeedsClip
	jae     @@inside
	mov     ss:SpanProc, offset ClipColumnDown
	jmp     ClippedColumnsLeftward
@@offView:
	DRAW_EXIT
@@inside:
	mov     ss:SpanProc, offset ColumnDown
EnlargeSideways ENDP

; Frame rows right to left. Each covers RowRepeat screen columns: the whole
; part of XFraction plus the scale.
ColumnsLeftward PROC FAR
	inc     ss:ScreenX
@@nextRow:
	xor     ax, ax
	add     ss:XFraction, bl
	adc     al, bh
	mov     ss:RowRepeat, al
	sub     ss:ScreenX, ax
@@nextSpan:
	mov     ax, [si].span_length
	or      ax, ax
	je      @@done
	mov     di, ss:SpanRow
	cmp     [si].span_y, di
	je      @@draw
	jg      @@rowDone
	add     si, SIZE SPANHDR
	shr     ax, 1
	jb      @@runs
	add     si, ax
	jmp     short @@skipped
@@runs:
	push    di
	mov     di, ax
@@run:
	lodsb
	inc     si
	shr     al, 1
	cbw
	jb      @@fill
	add     si, ax
	dec     si
@@fill:
	sub     di, ax
	jne     @@run
	pop     di
@@skipped:
	jmp     @@nextSpan
@@draw:
	call    ss:SpanProc
	jmp     @@nextSpan
@@rowDone:
	inc     ss:SpanRow
	dec     ss:FrameHeight
	jne     @@nextRow
@@done:
	DRAW_EXIT
ColumnsLeftward ENDP

; As ColumnsLeftward, skipping columns right of the clip box and stopping left
; of it.
ClippedColumnsLeftward PROC FAR
	mov     ax, ss:ScreenX
	inc     ax
	mov     ss:ScreenX, ax
	cmp     ax, ss:ClipRight
	jle     @@visible
	mov     dx, ss:ClipRight
@@beyond:
	xor     cx, cx
	add     ss:XFraction, bl
	adc     cl, bh
	sub     ax, cx
	cmp     ax, dx
	jle     @@reachesRight
	inc     ss:SpanRow
	dec     ss:FrameHeight
	jne     @@beyond
	jmp     @@done
@@reachesRight:
	mov     ss:ScreenX, ax
	sub     ax, dx
	or      ax, ax
	jns     @@positive
	neg     ax
@@positive:
	inc     ax
	jmp     short @@setRepeat
@@visible:
	xor     ax, ax
	add     ss:XFraction, bl
	adc     al, bh
	adc     ah, 0
	sub     ss:ScreenX, ax
	mov     cx, ss:ClipLeft
	sub     cx, ss:ScreenX
	jle     @@setRepeat
	sub     ax, cx
	mov     cx, ss:ClipLeft
	mov     ss:ScreenX, cx
@@setRepeat:
	mov     ss:RowRepeat, al
@@nextSpan:
	mov     ax, [si].span_length
	or      ax, ax
	je      @@done
	mov     di, ss:SpanRow
	cmp     [si].span_y, di
	je      @@draw
	jg      @@rowDone
	add     si, SIZE SPANHDR
	shr     ax, 1
	jb      @@runs
	add     si, ax
	jmp     short @@skipped
@@runs:
	push    di
	mov     di, ax
@@run:
	lodsb
	inc     si
	shr     al, 1
	cbw
	jb      @@fill
	add     si, ax
	dec     si
@@fill:
	sub     di, ax
	jne     @@run
	pop     di
@@skipped:
	jmp     @@nextSpan
@@draw:
	call    ss:SpanProc
	jmp     @@nextSpan
@@rowDone:
	inc     ss:SpanRow
	dec     ss:FrameHeight
	je      @@done
	mov     ax, ss:ScreenX
	cmp     ax, ss:ClipLeft
	jg      @@visible
@@done:
	DRAW_EXIT
ClippedColumnsLeftward ENDP

EnlargeSidewaysFlip PROC FAR
	call    far ptr PlaceEnlargedSidewaysFlip
	jb      @@offView
	call    far ptr FrameNeedsClip
	jae     @@inside
	mov     ss:SpanProc, offset ClipColumnDown
	jmp     ClippedColumnsRightward
@@offView:
	DRAW_EXIT
@@inside:
	mov     ss:SpanProc, offset ColumnDown
	jmp     ColumnsRightward
EnlargeSidewaysFlip ENDP

EnlargeSidewaysBoth PROC FAR
	call    far ptr PlaceEnlargedSidewaysBoth
	jb      @@offView
	mov     ax, ss:DrawBottom
	mov     ss:ScreenY, ax
	call    far ptr FrameNeedsClip
	jae     @@inside
	mov     ss:SpanProc, offset ClipColumnUp
	jmp     ClippedColumnsRightward
@@offView:
	DRAW_EXIT
@@inside:
	mov     ss:SpanProc, offset ColumnUp
EnlargeSidewaysBoth ENDP

; Frame rows left to right.
ColumnsRightward PROC FAR
	xor     ax, ax
	add     ss:XFraction, bl
	adc     al, bh
	mov     ss:RowRepeat, al
@@nextSpan:
	mov     ax, [si].span_length
	or      ax, ax
	je      @@done
	mov     di, ss:SpanRow
	cmp     [si].span_y, di
	je      @@draw
	jg      @@rowDone
	add     si, SIZE SPANHDR
	shr     ax, 1
	jb      @@runs
	add     si, ax
	jmp     short @@skipped
@@runs:
	push    di
	mov     di, ax
@@run:
	lodsb
	inc     si
	shr     al, 1
	cbw
	jb      @@fill
	add     si, ax
	dec     si
@@fill:
	sub     di, ax
	jne     @@run
	pop     di
@@skipped:
	jmp     @@nextSpan
@@draw:
	call    ss:SpanProc
	jmp     @@nextSpan
@@rowDone:
	xor     ax, ax
	mov     al, ss:RowRepeat
	add     ss:ScreenX, ax
	inc     ss:SpanRow
	dec     ss:FrameHeight
	jne     ColumnsRightward
@@done:
	DRAW_EXIT
ColumnsRightward ENDP

; As ColumnsRightward, skipping columns left of the clip box and stopping
; right of it.
ClippedColumnsRightward PROC FAR
	mov     ax, ss:ScreenX
	cmp     ax, ss:ClipLeft
	jge     @@visible
	mov     dx, ss:ClipLeft
@@before:
	xor     cx, cx
	add     ss:XFraction, bl
	adc     cl, bh
	add     ax, cx
	cmp     ax, dx
	jg      @@reachesLeft
	inc     ss:SpanRow
	dec     ss:FrameHeight
	jne     @@before
	jmp     @@done
@@reachesLeft:
	sub     ax, dx
	mov     ss:ScreenX, dx
	jmp     short @@setRepeat
@@visible:
	xor     ax, ax
	add     ss:XFraction, bl
	adc     al, bh
	adc     ah, 0
	mov     cx, ss:ClipRight
	inc     cx
	sub     cx, ax
	sub     cx, ss:ScreenX
	jge     @@setRepeat
	add     ax, cx
@@setRepeat:
	mov     ss:RowRepeat, al
@@nextSpan:
	mov     ax, [si].span_length
	or      ax, ax
	je      @@done
	mov     di, ss:SpanRow
	cmp     [si].span_y, di
	je      @@draw
	jg      @@rowDone
	add     si, SIZE SPANHDR
	shr     ax, 1
	jb      @@runs
	add     si, ax
	jmp     short @@skipped
@@runs:
	push    di
	mov     di, ax
@@run:
	lodsb
	inc     si
	shr     al, 1
	cbw
	jb      @@fill
	add     si, ax
	dec     si
@@fill:
	sub     di, ax
	jne     @@run
	pop     di
@@skipped:
	jmp     @@nextSpan
@@draw:
	call    ss:SpanProc
	jmp     @@nextSpan
@@rowDone:
	inc     ss:SpanRow
	dec     ss:FrameHeight
	je      @@done
	xor     ax, ax
	mov     al, ss:RowRepeat
	add     ss:ScreenX, ax
	mov     ax, ss:ScreenX
	cmp     ax, ss:ClipRight
	jle     @@visible
@@done:
	DRAW_EXIT
ClippedColumnsRightward ENDP

; Draw one span down the screen, RowRepeat pixels wide, each pixel as tall as
; the scale's whole part plus what the fraction in dl carries. A run-encoded
; span is first unpacked into SpanBuffer.
ColumnDown PROC NEAR
	shr     ax, 1
	mov     ss:RunLength, ax
	jae     @@raw
	mov     bx, ax
	mov     ax, [si].span_x
	add     si, SIZE SPANHDR
	push    ax
	push    es
	mov     ax, ss
	mov     es, ax
	mov     di, offset SpanBuffer
@@nextRun:
	lodsb
	shr     al, 1
	cbw
	mov     cx, ax
	jb      @@fill
	shr     cx, 1
	rep     movsw
	rcl     cx, 1
	rep     movsb
	sub     bx, ax
	jne     @@nextRun
	je      @@unpacked
@@fill:
	sub     bx, ax
	lodsb
	mov     ah, al
	shr     cx, 1
	rep     stosw
	rcl     cx, 1
	rep     stosb
	or      bx, bx
	jne     @@nextRun
@@unpacked:
	pop     es
	pop     ax
	push    si
	push    ds
	push    ss
	pop     ds
	mov     bx, CurrentScale
	mov     si, offset SpanBuffer
	jmp     short @@place
@@raw:
	mov     di, ax
	mov     ax, [si].span_x
	add     si, SIZE SPANHDR
	add     di, si
	push    di
	push    ds
@@place:
	add     ax, ss:FrameLeft
	mul     bx
	add     al, ss:YFraction
	mov     cl, al
	adc     ah, 0
	adc     dx, 0
	mov     al, ah
	mov     ah, dl
	mov     dl, cl
	add     ax, ss:ScreenY
	shl     ax, 1
	add     ax, ss:ViewRows
	mov     di, ax
	mov     di, ss:[di]
	add     di, ss:ScreenX
	mov     ah, byte ptr ss:RunLength
	xor     cx, cx
	mov     cl, ss:RowRepeat
	mov     bp, cx
	xor     dh, dh
@@pixel:
	add     dl, bl
	adc     dh, bh
	lodsb
@@pixelRow:
	rep     stosb
	mov     cx, bp
	sub     di, cx
	add     di, ss:RowPitch
	dec     dh
	jne     @@pixelRow
	dec     ah
	jne     @@pixel
	pop     ds
	pop     si
	ret
ColumnDown ENDP

; As ColumnDown, up the screen.
ColumnUp PROC NEAR
	shr     ax, 1
	mov     ss:RunLength, ax
	jae     @@raw
	mov     bx, ax
	mov     ax, [si].span_x
	add     si, SIZE SPANHDR
	push    ax
	push    es
	mov     ax, ss
	mov     es, ax
	mov     di, offset SpanBuffer
@@nextRun:
	lodsb
	shr     al, 1
	cbw
	mov     cx, ax
	jb      @@fill
	shr     cx, 1
	rep     movsw
	rcl     cx, 1
	rep     movsb
	sub     bx, ax
	jne     @@nextRun
	je      @@unpacked
@@fill:
	sub     bx, ax
	lodsb
	mov     ah, al
	shr     cx, 1
	rep     stosw
	rcl     cx, 1
	rep     stosb
	or      bx, bx
	jne     @@nextRun
@@unpacked:
	pop     es
	pop     ax
	push    si
	push    ds
	push    ss
	pop     ds
	mov     bx, CurrentScale
	mov     si, offset SpanBuffer
	jmp     short @@place
@@raw:
	mov     di, ax
	mov     ax, [si].span_x
	add     si, SIZE SPANHDR
	add     di, si
	push    di
	push    ds
@@place:
	add     ax, ss:FrameLeft
	mul     bx
	add     al, ss:YFraction
	mov     cl, al
	adc     ah, 0
	adc     dx, 0
	mov     al, ah
	mov     ah, dl
	mov     dl, cl
	neg     ax
	add     ax, ss:ScreenY
	shl     ax, 1
	add     ax, ss:ViewRows
	mov     di, ax
	mov     di, ss:[di]
	add     di, ss:ScreenX
	mov     ah, byte ptr ss:RunLength
	xor     cx, cx
	mov     cl, ss:RowRepeat
	mov     bp, cx
	xor     dh, dh
@@pixel:
	add     dl, bl
	adc     dh, bh
	lodsb
@@pixelRow:
	rep     stosb
	mov     cx, bp
	sub     di, cx
	sub     di, ss:RowPitch
	dec     dh
	jne     @@pixelRow
	dec     ah
	jne     @@pixel
	pop     ds
	pop     si
	ret
ColumnUp ENDP

; As ColumnDown, clipped to the clip box's top and bottom.
ClipColumnDown PROC NEAR
	shr     ax, 1
	mov     ss:RunLength, ax
	jae     @@raw
	mov     bx, ax
	mov     ax, [si].span_x
	add     si, SIZE SPANHDR
	push    ax
	push    es
	mov     ax, ss
	mov     es, ax
	mov     di, offset SpanBuffer
@@nextRun:
	lodsb
	shr     al, 1
	cbw
	mov     cx, ax
	jb      @@fill
	shr     cx, 1
	rep     movsw
	rcl     cx, 1
	rep     movsb
	sub     bx, ax
	jne     @@nextRun
	je      @@unpacked
@@fill:
	sub     bx, ax
	lodsb
	mov     ah, al
	shr     cx, 1
	rep     stosw
	rcl     cx, 1
	rep     stosb
	or      bx, bx
	jne     @@nextRun
@@unpacked:
	pop     es
	pop     ax
	push    si
	push    ds
	push    ss
	pop     ds
	mov     bx, CurrentScale
	mov     si, offset SpanBuffer
	jmp     short @@place
@@raw:
	mov     di, ax
	mov     ax, [si].span_x
	add     si, SIZE SPANHDR
	add     di, si
	push    di
	push    ds
@@place:
	add     ax, ss:FrameLeft
	mul     bx
	add     al, ss:YFraction
	mov     cl, al
	adc     ah, 0
	adc     dx, 0
	mov     al, ah
	mov     ah, dl
	add     ax, ss:ScreenY
	mov     di, ax
	mov     ss:ClipEdges, 0
	cmp     ax, ss:ClipBottom
	jle     @@startsAbove
@@toDone:
	jmp     @@done
@@startsAbove:
	mov     ax, ss:RunLength
	mul     bx
	add     al, cl
	adc     ah, 0
	adc     dx, 0
	mov     al, ah
	mov     ah, dl
	add     ax, di
	cmp     ax, ss:ClipTop
	jl      @@toDone
	cmp     ax, ss:ClipBottom
	jle     @@bottomIn
; count only the pixels down to the bottom edge
	mov     ax, ss:ClipBottom
	inc     ax
	sub     ax, di
	xor     dx, dx
	mov     dl, ah
	mov     ah, al
	xor     al, al
	sub     al, cl
	sbb     ah, 0
	sbb     dx, 0
	div     bx
	or      dx, dx
	je      @@bottomCount
	or      ss:ClipEdges, CLIP_BOTTOM
@@bottomCount:
	mov     ss:RunLength, ax
	mul     bx
	add     al, cl
	adc     ah, 0
	adc     dx, 0
	mov     al, ah
	mov     ah, dl
	add     ax, di
	mov     LineY, ax
@@bottomIn:
	mov     ax, di
	sub     ax, ss:ClipTop
	jge     @@topIn
; skip the pixels above the top edge
	or      ax, ax
	jns     @@topCount
	neg     ax
@@topCount:
	xor     dx, dx
	mov     dl, ah
	mov     ah, al
	xor     al, al
	sub     al, cl
	sbb     ah, 0
	sbb     dx, 0
	div     bx
	sub     ss:RunLength, ax
	add     si, ax
	mul     bx
	add     al, cl
	mov     cl, al
	adc     ah, 0
	adc     dx, 0
	mov     al, ah
	mov     ah, dl
	add     ax, di
	mov     di, ss:ClipTop
	sub     ax, di
	mov     dx, cx
	add     dl, bl
	adc     al, bh
	xor     cx, cx
	mov     dh, al
	jmp     short @@draw
@@topIn:
	mov     dx, cx
	xor     dh, dh
	add     dl, bl
	adc     dh, bh
@@draw:
	shl     di, 1
	add     di, ss:ViewRows
	mov     di, ss:[di]
	add     di, ss:ScreenX
	xor     cx, cx
	mov     cl, ss:RowRepeat
	mov     bp, cx
	lodsb
	mov     ah, byte ptr ss:RunLength
	or      ah, ah
	jle     @@pixelsDone
	or      dh, dh
	je      @@nextPixel
@@pixelRow:
	rep     stosb
	mov     cx, bp
	sub     di, cx
	add     di, ss:RowPitch
	dec     dh
	jg      @@pixelRow
@@nextPixel:
	lodsb
	add     dl, bl
	adc     dh, bh
	dec     ah
	jne     @@pixelRow
@@pixelsDone:
; stretch the cut last pixel to the bottom edge
	test    ss:ClipEdges, CLIP_BOTTOM
	je      @@done
	mov     dx, ss:ClipBottom
	inc     dx
	sub     dx, LineY
	mov     dh, dl
@@lastRow:
	rep     stosb
	mov     cx, bp
	sub     di, cx
	add     di, ss:RowPitch
	dec     dh
	jne     @@lastRow
@@done:
	pop     ds
	pop     si
	ret
ClipColumnDown ENDP

; As ColumnUp, clipped to the clip box's top and bottom.
ClipColumnUp PROC NEAR
	shr     ax, 1
	mov     ss:RunLength, ax
	jae     @@raw
	mov     bx, ax
	mov     ax, [si].span_x
	add     si, SIZE SPANHDR
	push    ax
	push    es
	mov     ax, ss
	mov     es, ax
	mov     di, offset SpanBuffer
@@nextRun:
	lodsb
	shr     al, 1
	cbw
	mov     cx, ax
	jb      @@fill
	shr     cx, 1
	rep     movsw
	rcl     cx, 1
	rep     movsb
	sub     bx, ax
	jne     @@nextRun
	je      @@unpacked
@@fill:
	sub     bx, ax
	lodsb
	mov     ah, al
	shr     cx, 1
	rep     stosw
	rcl     cx, 1
	rep     stosb
	or      bx, bx
	jne     @@nextRun
@@unpacked:
	pop     es
	pop     ax
	push    si
	push    ds
	push    ss
	pop     ds
	mov     bx, CurrentScale
	mov     si, offset SpanBuffer
	jmp     short @@place
@@raw:
	mov     di, ax
	mov     ax, [si].span_x
	add     si, SIZE SPANHDR
	add     di, si
	push    di
	push    ds
@@place:
	add     ax, ss:FrameLeft
	mul     bx
	add     al, ss:YFraction
	mov     cl, al
	adc     ah, 0
	adc     dx, 0
	mov     al, ah
	mov     ah, dl
	neg     ax
	add     ax, ss:ScreenY
	mov     di, ax
	mov     ss:ClipEdges, 0
	cmp     ax, ss:ClipTop
	jge     @@startsBelow
@@toDone:
	jmp     @@done
@@startsBelow:
	mov     ax, ss:RunLength
	mul     bx
	add     al, cl
	adc     ah, 0
	adc     dx, 0
	mov     al, ah
	mov     ah, dl
	neg     ax
	add     ax, di
	cmp     ax, ss:ClipBottom
	jg      @@toDone
	cmp     ax, ss:ClipTop
	jge     @@topIn
; count only the pixels up to the top edge
	mov     ax, di
	inc     ax
	sub     ax, ss:ClipTop
	xor     dx, dx
	mov     dl, ah
	mov     ah, al
	xor     al, al
	sub     al, cl
	sbb     ah, 0
	sbb     dx, 0
	div     bx
	or      dx, dx
	je      @@topCount
	or      ss:ClipEdges, CLIP_TOP
@@topCount:
	mov     ss:RunLength, ax
	mul     bx
	add     al, cl
	adc     ah, 0
	adc     dx, 0
	mov     al, ah
	mov     ah, dl
	neg     ax
	add     ax, di
	mov     LineY, ax
@@topIn:
	mov     ax, di
	sub     ax, ss:ClipBottom
	jle     @@bottomIn
; skip the pixels below the bottom edge
	xor     dx, dx
	mov     dl, ah
	mov     ah, al
	xor     al, al
	sub     al, cl
	sbb     ah, 0
	sbb     dx, 0
	div     bx
	sub     ss:RunLength, ax
	add     si, ax
	mul     bx
	add     al, cl
	mov     cl, al
	adc     ah, 0
	adc     dx, 0
	mov     al, ah
	mov     ah, dl
	neg     ax
	add     ax, di
	mov     di, ss:ClipBottom
	sub     ax, di
	neg     ax
	mov     dx, cx
	add     dl, bl
	adc     al, bh
	xor     cx, cx
	mov     dh, al
	jmp     short @@draw
@@bottomIn:
	mov     dx, cx
	xor     dh, dh
	add     dl, bl
	adc     dh, bh
@@draw:
	shl     di, 1
	add     di, ss:ViewRows
	mov     di, ss:[di]
	add     di, ss:ScreenX
	xor     cx, cx
	mov     cl, ss:RowRepeat
	mov     bp, cx
	lodsb
	mov     ah, byte ptr ss:RunLength
	or      ah, ah
	jle     @@pixelsDone
	or      dh, dh
	je      @@nextPixel
@@pixelRow:
	rep     stosb
	mov     cx, bp
	sub     di, cx
	sub     di, ss:RowPitch
	dec     dh
	jg      @@pixelRow
@@nextPixel:
	lodsb
	add     dl, bl
	adc     dh, bh
	dec     ah
	jne     @@pixelRow
@@pixelsDone:
; stretch the cut last pixel to the top edge
	test    ss:ClipEdges, CLIP_TOP
	je      @@done
	mov     dx, LineY
	inc     dx
	sub     dx, ss:ClipTop
	mov     dh, dl
@@lastRow:
	rep     stosb
	mov     cx, bp
	sub     di, cx
	sub     di, ss:RowPitch
	dec     dh
	jne     @@lastRow
@@done:
	pop     ds
	pop     si
	ret

ClipColumnUp ENDP

	END
