; Black Gate INTRO.EXE, resident segment 76 (file offsets 0x017c42 to 0x01843d, 2043 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

	.MODEL  MEDIUM
	LOCALS

	INCLUDE scalfram.inc

	.CODE

; Draw a shape frame enlarged (scale 100h and up) and not turned, one routine
; per flip. Each places the frame, picks a span routine and goes to a row loop.
; bx is the scale in 256ths throughout.
EnlargeFrameMirror PROC FAR
	call    far ptr PlaceEnlargedMirror
	jb      @@offView
	mov     ax, ss:DrawRight
	mov     ss:ScreenX, ax
	call    far ptr FrameNeedsClip
	jae     @@inside
	mov     ss:SpanProc, offset ClipSpanMirror
	jmp     ClippedRowsDown
@@offView:
	DRAW_EXIT
@@inside:
	mov     ss:SpanProc, offset PlainSpanMirror
	jmp     RowsDown
EnlargeFrameMirror ENDP

EnlargeFrame PROC FAR
	call    far ptr PlaceEnlarged
	jb      @@offView
	call    far ptr FrameNeedsClip
	jae     @@inside
	mov     ss:SpanProc, offset ClipSpan
	jmp     ClippedRowsDown
@@offView:
	DRAW_EXIT
@@inside:
	mov     ss:SpanProc, offset PlainSpan
EnlargeFrame ENDP

; Top to bottom. Each frame row covers RowRepeat screen rows: the whole part
; of YFraction plus the scale. ScreenY becomes a pointer into the row table.
RowsDown PROC FAR
	mov     ax, ss:ScreenY
	shl     ax, 1
	add     ax, ss:ViewRows
	mov     ss:ScreenY, ax
@@nextRow:
	xor     ax, ax
	add     ss:YFraction, bl
	adc     al, bh
	mov     ss:RowRepeat, al
@@nextSpan:
	mov     ax, [si].span_length
	or      ax, ax
	jne     @@inSpan
	jmp     @@done
@@inSpan:
	mov     di, ss:SpanRow
	cmp     [si].span_y, di
	je      @@draw
	jl      @@skip
	jmp     @@rowDone
@@skip:
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
	shl     ax, 1
	add     ss:ScreenY, ax
	jmp     @@nextRow
@@done:
	DRAW_EXIT
RowsDown ENDP

; As RowsDown, skipping rows above the clip box and stopping below it.
ClippedRowsDown PROC FAR
	mov     ax, ss:ScreenY
	cmp     ax, ss:ClipTop
	jge     @@visible
	mov     dx, ss:ClipTop
@@above:
	xor     cx, cx
	add     ss:YFraction, bl
	adc     cl, bh
	add     ax, cx
	cmp     ax, dx
	jg      @@reachesTop
	inc     ss:SpanRow
	dec     ss:FrameHeight
	jne     @@above
	jmp     @@done
@@reachesTop:
	mov     ss:ScreenY, dx
	sub     ax, dx
	jmp     short @@setRepeat
@@visible:
	xor     ax, ax
	add     ss:YFraction, bl
	adc     al, bh
@@setRepeat:
	mov     ss:RowRepeat, al
@@nextSpan:
	mov     ax, [si].span_length
	or      ax, ax
	jne     @@inSpan
	jmp     @@done
@@inSpan:
	mov     di, ss:SpanRow
	cmp     [si].span_y, di
	je      @@draw
	jl      @@skip
	jmp     @@rowDone
@@skip:
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
	add     ax, ss:ScreenY
	mov     ss:ScreenY, ax
	cmp     ax, ss:ClipBottom
	jle     @@visible
@@done:
	DRAW_EXIT
ClippedRowsDown ENDP

EnlargeFrameFlip PROC FAR
	call    far ptr PlaceEnlargedFlip
	jb      @@offView
	call    far ptr FrameNeedsClip
	jae     @@inside
	mov     ss:SpanProc, offset ClipSpan
	jmp     ClippedRowsUp
@@offView:
	DRAW_EXIT
@@inside:
	mov     ss:SpanProc, offset PlainSpan
	jmp     RowsUp
EnlargeFrameFlip ENDP

EnlargeFrameBoth PROC FAR
	call    far ptr PlaceEnlargedBoth
	jb      @@offView
	mov     ax, ss:DrawRight
	mov     ss:ScreenX, ax
	call    far ptr FrameNeedsClip
	jae     @@inside
	mov     ss:SpanProc, offset ClipSpanMirror
	jmp     ClippedRowsUp
@@offView:
	DRAW_EXIT
@@inside:
	mov     ss:SpanProc, offset PlainSpanMirror
EnlargeFrameBoth ENDP

; Bottom to top: the frame's first row lands on the lowest screen rows.
RowsUp PROC FAR
	mov     ax, ss:DrawBottom
	inc     ax
	shl     ax, 1
	add     ax, ss:ViewRows
	mov     ss:ScreenY, ax
@@nextRow:
	xor     ax, ax
	add     ss:YFraction, bl
	adc     al, bh
	mov     ss:RowRepeat, al
	shl     ax, 1
	sub     ss:ScreenY, ax
@@nextSpan:
	mov     ax, [si].span_length
	or      ax, ax
	jne     @@inSpan
	jmp     @@done
@@inSpan:
	mov     di, ss:SpanRow
	cmp     [si].span_y, di
	je      @@draw
	jl      @@skip
	jmp     @@rowDone
@@skip:
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
RowsUp ENDP

; As RowsUp, skipping rows below the clip box and stopping above it.
ClippedRowsUp PROC FAR
	mov     ax, ss:DrawBottom
	inc     ax
	mov     ss:ScreenY, ax
	cmp     ax, ss:ClipBottom
	jle     @@visible
	mov     dx, ss:ClipBottom
@@below:
	xor     cx, cx
	add     ss:YFraction, bl
	adc     cl, bh
	sub     ax, cx
	cmp     ax, dx
	jle     @@reachesBottom
	inc     ss:SpanRow
	dec     ss:FrameHeight
	jne     @@below
	jmp     @@done
@@reachesBottom:
	mov     ss:ScreenY, ax
	mov     cx, ax
	jmp     short @@setRepeat
@@visible:
	xor     ax, ax
	add     ss:YFraction, bl
	adc     al, bh
	sub     ss:ScreenY, ax
	mov     dx, ss:ClipTop
	sub     dx, ss:ScreenY
	jle     @@setRepeat
	sub     ax, dx
	mov     dx, ss:ClipTop
	mov     ss:ScreenY, dx
@@setRepeat:
	mov     ss:RowRepeat, al
@@nextSpan:
	mov     ax, [si].span_length
	or      ax, ax
	je      @@done
	mov     di, ss:SpanRow
	cmp     [si].span_y, di
	je      @@draw
	jl      @@skip
	jmp     short @@rowDone
@@skip:
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
	mov     ax, ss:ScreenY
	cmp     ax, ss:ClipTop
	jg      @@visible
@@done:
	DRAW_EXIT
ClippedRowsUp ENDP

; Draw one span left to right, each pixel as wide as the scale's whole part
; plus what the fraction in dl carries, then copy the row down RowRepeat - 1
; more rows. bp counts the pixels stored.
PlainSpan PROC NEAR
	mov     ax, [si].span_x
	add     ax, ss:FrameLeft
	mul     bx
	add     al, ss:XFraction
	mov     cl, al
	adc     ah, 0
	adc     dx, 0
	mov     al, ah
	mov     ah, dl
	mov     dl, cl
	add     ax, ss:ScreenX
	mov     ss:SpanStart, ax
	mov     di, ss:ScreenY
	mov     di, ss:[di]
	add     di, ax
	xor     bp, bp
	mov     ax, [si].span_length
	add     si, SIZE SPANHDR
	shr     ax, 1
	jb      @@runs
	mov     ah, al
	xor     cx, cx
@@pixel:
	lodsb
	add     dl, bl
	adc     cl, bh
	add     bp, cx
	rep     stosb
	dec     ah
	jne     @@pixel
; the rows below repeat this one
@@repeatRows:
	mov     dh, ss:RowRepeat
	dec     dh
	je      @@done
	push    si
	push    ds
	push    es
	pop     ds
	mov     ax, bp
	mov     bp, ss:ScreenY
@@copyRow:
	mov     si, ss:SpanStart
	mov     di, si
	add     si, [bp]
	add     bp, 2
	add     di, [bp]
	mov     cx, ax
	shr     cx, 1
	rep     movsw
	rcl     cx, 1
	rep     movsb
	dec     dh
	jne     @@copyRow
	pop     ds
	pop     si
@@done:
	ret
@@runs:
	mov     bx, ax
@@nextRun:
	lodsb
	shr     al, 1
	cbw
	jb      @@fill
	sub     bx, ax
	mov     ah, al
	xor     cx, cx
@@copy:
	lodsb
	add     dl, byte ptr ss:CurrentScale
	adc     cl, byte ptr ss:CurrentScale+1
	adc     ch, 0
	add     bp, cx
	rep     stosb
	dec     ah
	jne     @@copy
	or      bl, bl
	jne     @@nextRun
	jmp     short @@spanDone
@@fill:
	sub     bx, ax
	mov     cx, dx
	mul     ss:CurrentScale
	add     cl, al
	adc     ah, 0
	adc     dx, 0
	mov     al, ah
	mov     ah, dl
	mov     dx, cx
	mov     cx, ax
	lodsb
	mov     ah, al
	add     bp, cx
	shr     cx, 1
	rep     stosw
	rcl     cx, 1
	rep     stosb
	or      bl, bl
	jne     @@nextRun
@@spanDone:
	mov     bx, ss:CurrentScale
	jmp     @@repeatRows
PlainSpan ENDP

; As PlainSpan, right to left.
PlainSpanMirror PROC NEAR
	mov     ax, [si].span_x
	add     ax, ss:FrameLeft
	mul     bx
	add     al, ss:XFraction
	mov     cl, al
	adc     ah, 0
	adc     dx, 0
	mov     al, ah
	mov     ah, dl
	mov     dl, cl
	neg     ax
	add     ax, ss:ScreenX
	mov     ss:SpanStart, ax
	mov     di, ss:ScreenY
	mov     di, ss:[di]
	add     di, ax
	xor     bp, bp
	std
	mov     ax, [si].span_length
	add     si, SIZE SPANHDR
	shr     ax, 1
	jb      @@runs
	mov     ah, al
	xor     cx, cx
@@pixel:
	mov     al, [si]
	inc     si
	add     dl, bl
	adc     cl, bh
	add     bp, cx
	rep     stosb
	dec     ah
	jne     @@pixel
; the rows below repeat this one
@@repeatRows:
	mov     dh, ss:RowRepeat
	dec     dh
	je      @@done
	push    si
	push    ds
	push    es
	pop     ds
	mov     ax, bp
	mov     bp, ss:ScreenY
@@copyRow:
	mov     si, ss:SpanStart
	mov     di, si
	add     si, [bp]
	add     bp, 2
	add     di, [bp]
	mov     cx, ax
	rep     movsb
	dec     dh
	jne     @@copyRow
	pop     ds
	pop     si
@@done:
	cld
	ret
@@runs:
	mov     bx, ax
@@nextRun:
	mov     al, [si]
	inc     si
	shr     al, 1
	cbw
	jb      @@fill
	sub     bx, ax
	mov     ah, al
	xor     cx, cx
@@copy:
	mov     al, [si]
	inc     si
	add     dl, byte ptr ss:CurrentScale
	adc     cl, byte ptr ss:CurrentScale+1
	add     bp, cx
	rep     stosb
	dec     ah
	jne     @@copy
	or      bl, bl
	jne     @@nextRun
	jmp     short @@spanDone
@@fill:
	sub     bx, ax
	mov     cx, dx
	mul     ss:CurrentScale
	add     cl, al
	adc     ah, 0
	adc     dx, 0
	mov     al, ah
	mov     ah, dl
	mov     dx, cx
	mov     cx, ax
	mov     al, [si]
	inc     si
	mov     ah, al
	add     bp, cx
	rep     stosb
	or      bl, bl
	jne     @@nextRun
@@spanDone:
	mov     bx, ss:CurrentScale
	jmp     @@repeatRows
PlainSpanMirror ENDP

; Draw one span clipped to the clip box. A run-encoded span is first unpacked
; into SpanBuffer. dh is set when the last pixel is cut by the right edge.
ClipSpan PROC NEAR
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
	xor     ch, ch
	mul     bx
	add     al, ss:XFraction
	mov     cl, al
	adc     ah, 0
	adc     dx, 0
	mov     al, ah
	mov     ah, dl
	add     ax, ss:ScreenX
	mov     di, ax
	cmp     ax, ss:ClipRight
	jle     @@startsLeft
@@toDone:
	jmp     @@done
@@startsLeft:
	mov     ax, ss:RunLength
	mul     bx
	add     al, cl
	adc     ah, 0
	adc     dx, 0
	mov     al, ah
	mov     ah, dl
	add     ax, di
	cmp     ax, ss:ClipLeft
	jl      @@toDone
	cmp     ax, ss:ClipRight
	jle     @@rightIn
; count only the pixels up to the right edge
	mov     ax, ss:ClipRight
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
	je      @@rightCount
	mov     ch, 1
@@rightCount:
	mov     ss:RunLength, ax
@@rightIn:
	mov     ax, di
	sub     ax, ss:ClipLeft
	jge     @@leftIn
; skip the pixels left of the left edge
	or      ax, ax
	jns     @@leftCount
	neg     ax
@@leftCount:
	xor     dx, dx
	mov     dl, ah
	mov     ah, al
	xor     al, al
	sub     al, cl
	sbb     ah, 0
	sbb     dx, 0
	jb      @@toDone
	div     bx
	sub     ss:RunLength, ax
	jb      @@toDone
	add     si, ax
	mul     bx
	add     al, cl
	mov     cl, al
	adc     ah, 0
	adc     dx, 0
	mov     al, ah
	mov     ah, dl
	add     ax, di
	mov     di, ss:ClipLeft
	sub     ax, di
	mov     dx, cx
	add     dl, bl
	adc     al, bh
	xor     cx, cx
	mov     cl, al
	jmp     short @@draw
@@leftIn:
	mov     dx, cx
	xor     cx, cx
	add     dl, bl
	adc     cl, bh
@@draw:
	mov     ax, di
	mov     di, ss:ScreenY
	shl     di, 1
	add     di, ss:ViewRows
	mov     di, ss:[di]
	add     di, ax
	push    ax
	mov     ss:SpanStart, di
	xor     bp, bp
	mov     ah, byte ptr ss:RunLength
	or      ah, ah
	je      @@pixelsDone
@@pixel:
	lodsb
	add     bp, cx
	rep     stosb
	add     dl, bl
	adc     cl, bh
	dec     ah
	jne     @@pixel
@@pixelsDone:
	pop     ax
; widen the cut last pixel to reach the right edge
	or      dh, dh
	je      @@repeatRows
	add     ax, bp
	stc
	sbb     ax, ss:ClipRight
	or      ax, ax
	jns     @@lastPixel
	neg     ax
@@lastPixel:
	mov     cx, ax
	add     bp, cx
	lodsb
	rep     stosb
; the rows below repeat this one
@@repeatRows:
	mov     dh, ss:RowRepeat
	dec     dh
	je      @@done
	push    es
	pop     ds
	mov     ax, ss:ScreenY
@@copyRow:
	inc     ax
	cmp     ax, ss:ClipBottom
	jg      @@done
	mov     si, ss:SpanStart
	mov     di, si
	add     di, ss:RowPitch
	mov     ss:SpanStart, di
	mov     cx, bp
	shr     cx, 1
	rep     movsw
	rcl     cx, 1
	rep     movsb
	dec     dh
	jne     @@copyRow
@@done:
	pop     ds
	pop     si
	ret
ClipSpan ENDP

; As ClipSpan, right to left.
ClipSpanMirror PROC NEAR
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
	xor     ch, ch
	mul     bx
	add     al, ss:XFraction
	mov     cl, al
	adc     ah, 0
	adc     dx, 0
	mov     al, ah
	mov     ah, dl
	neg     ax
	add     ax, ss:ScreenX
	mov     di, ax
	cmp     ax, ss:ClipLeft
	jge     @@startsRight
@@toDone:
	jmp     @@done
@@startsRight:
	mov     ax, ss:RunLength
	mul     bx
	add     al, cl
	adc     ah, 0
	adc     dx, 0
	mov     al, ah
	mov     ah, dl
	neg     ax
	add     ax, di
	cmp     ax, ss:ClipRight
	jg      @@toDone
	cmp     ax, ss:ClipLeft
	jge     @@leftIn
; count only the pixels down to the left edge
	mov     ax, di
	inc     ax
	sub     ax, ss:ClipLeft
	xor     dx, dx
	mov     dl, ah
	mov     ah, al
	xor     al, al
	sub     al, cl
	sbb     ah, 0
	sbb     dx, 0
	div     bx
	or      dx, dx
	je      @@leftCount
	mov     ch, 1
@@leftCount:
	mov     ss:RunLength, ax
@@leftIn:
	mov     ax, di
	sub     ax, ss:ClipRight
	jle     @@rightIn
; skip the pixels right of the right edge
	xor     dx, dx
	mov     dl, ah
	mov     ah, al
	xor     al, al
	sub     al, cl
	sbb     ah, 0
	sbb     dx, 0
	div     bx
	sub     ss:RunLength, ax
	jle     @@toDone
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
	mov     di, ss:ClipRight
	sub     ax, di
	neg     ax
	mov     dx, cx
	add     dl, bl
	adc     al, bh
	xor     cx, cx
	mov     cl, al
	jmp     short @@draw
@@rightIn:
	mov     dx, cx
	xor     cx, cx
	add     dl, bl
	adc     cl, bh
@@draw:
	mov     ax, di
	mov     di, ss:ScreenY
	shl     di, 1
	add     di, ss:ViewRows
	mov     di, ss:[di]
	add     di, ax
	push    ax
	mov     ss:SpanStart, di
	std
	xor     bp, bp
	mov     ah, byte ptr ss:RunLength
	or      ah, ah
	je      @@pixelsDone
@@pixel:
	mov     al, [si]
	inc     si
	add     bp, cx
	rep     stosb
	add     dl, bl
	adc     cl, bh
	dec     ah
	jne     @@pixel
@@pixelsDone:
	pop     ax
; widen the cut last pixel to reach the left edge
	or      dh, dh
	je      @@repeatRows
	sub     ax, bp
	inc     ax
	sub     ax, ss:ClipLeft
	mov     cx, ax
	add     bp, cx
	mov     al, [si]
	inc     si
	rep     stosb
; the rows below repeat this one
@@repeatRows:
	mov     dh, ss:RowRepeat
	dec     dh
	je      @@done
	push    es
	pop     ds
	mov     ax, ss:ScreenY
@@copyRow:
	inc     ax
	cmp     ax, ss:ClipBottom
	jg      @@done
	mov     si, ss:SpanStart
	mov     di, si
	add     di, ss:RowPitch
	mov     ss:SpanStart, di
	mov     cx, bp
	rep     movsb
	dec     dh
	jne     @@copyRow
@@done:
	cld
	pop     ds
	pop     si
	ret

ClipSpanMirror ENDP

	END
