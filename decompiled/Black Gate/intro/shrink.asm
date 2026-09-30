; Black Gate INTRO.EXE, resident segment 75 (file offsets 0x017654 to 0x017c42, 1518 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

; Draws a shape frame shrunk (scale below 100h) and not turned, in each of
; the four flips.

	.MODEL  MEDIUM
	LOCALS

	INCLUDE scalfram.inc

	.CODE

ShrinkFrame PROC FAR
	call    far ptr PlaceFrame
	jb      @@offView
	call    far ptr FrameNeedsClip
	jae     @@inside
	mov     word ptr ss:SpanProc, offset ClipSpan
	jmp     ClippedRowsDown
@@offView:
	DRAW_EXIT
@@inside:
	mov     word ptr ss:SpanProc, offset PlainSpan
ShrinkFrame ENDP

; Top to bottom. dh is the scale in 256ths and ch the row fraction: a frame
; row reaches the screen only when adding the scale carries.
RowsDown PROC FAR
	mov     ax, word ptr ss:ScreenY
	mov     bp, ax
	shl     bp, 1
	add     bp, word ptr ss:ViewRows
	mov     ch, ss:YFraction
	mov     cl, byte ptr ss:FrameHeight
@@nextRow:
	add     ch, dh
	jb      @@drawRow
	inc     word ptr ss:SpanRow
	dec     cl
	jne     @@nextRow
	DRAW_EXIT
@@drawRow:
	mov     di, word ptr ss:SpanRow
@@nextSpan:
	mov     ax, [si].span_length
	or      ax, ax
	je      @@done
	cmp     [si].span_y, di
	je      @@draw
	jg      @@rowDone
; a span of a row that shrank away
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
	call    word ptr ss:SpanProc
	jmp     @@drawRow
@@rowDone:
	inc     word ptr ss:SpanRow
	add     bp, 2
	dec     cl
	je      @@done
	jmp     @@nextRow
@@done:
	DRAW_EXIT
RowsDown ENDP

; As RowsDown, skipping rows above the clip box and stopping below it.
ClippedRowsDown PROC FAR
	mov     ax, word ptr ss:ScreenY
	mov     bp, ax
	shl     bp, 1
	add     bp, word ptr ss:ViewRows
	mov     ch, ss:YFraction
	mov     cl, byte ptr ss:FrameHeight
	cmp     ax, word ptr ss:ClipTop
	jge     @@visible
@@above:
	inc     word ptr ss:SpanRow
	add     ch, dh
	jae     @@hidden
	add     bp, 2
	inc     ax
	mov     word ptr ss:ScreenY, ax
	cmp     ax, word ptr ss:ClipTop
	jge     @@visible
@@hidden:
	dec     cl
	jne     @@above
	je      @@done
@@visible:
	add     ch, dh
	jb      @@drawRow
	inc     word ptr ss:SpanRow
	dec     cl
	jne     @@visible
@@done:
	DRAW_EXIT
@@drawRow:
	mov     di, word ptr ss:SpanRow
@@nextSpan:
	mov     ax, [si].span_length
	or      ax, ax
	je      @@done
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
	call    word ptr ss:SpanProc
	jmp     @@drawRow
@@rowDone:
	inc     word ptr ss:SpanRow
	add     bp, 2
	inc     word ptr ss:ScreenY
	mov     ax, word ptr ss:ScreenY
	cmp     ax, word ptr ss:ClipBottom
	jg      @@below
	dec     cl
	jne     @@visible
@@below:
	DRAW_EXIT
ClippedRowsDown ENDP

ShrinkFrameMirror PROC FAR
	call    far ptr PlaceFrameMirror
	jb      @@offView
	call    far ptr FrameNeedsClip
	jae     @@inside
	mov     word ptr ss:SpanProc, offset ClipSpanMirror
	jmp     ClippedRowsDown
@@inside:
	mov     word ptr ss:SpanProc, offset PlainSpanMirror
	mov     bx, word ptr ss:DrawRight
	jmp     RowsDown
@@offView:
	DRAW_EXIT
ShrinkFrameMirror ENDP

ShrinkFrameFlip PROC FAR
	call    far ptr PlaceFrameFlip
	jb      @@offView
	call    far ptr FrameNeedsClip
	jae     @@inside
	mov     word ptr ss:SpanProc, offset ClipSpan
	jmp     ClippedRowsUp
@@inside:
	mov     word ptr ss:SpanProc, offset PlainSpan
	mov     bx, word ptr ss:DrawLeft
	jmp     RowsUp
@@offView:
	DRAW_EXIT
ShrinkFrameFlip ENDP

ShrinkFrameBoth PROC FAR
	call    far ptr PlaceFrameBoth
	jb      @@offView
	call    far ptr FrameNeedsClip
	jae     @@inside
	mov     word ptr ss:SpanProc, offset ClipSpanMirror
	jmp     ClippedRowsUp
@@inside:
	mov     word ptr ss:SpanProc, offset PlainSpanMirror
	mov     bx, word ptr ss:DrawRight
	jmp     RowsUp
@@offView:
	DRAW_EXIT
ShrinkFrameBoth ENDP

; Bottom to top: the frame's first row lands on the lowest screen row.
RowsUp PROC FAR
	mov     ax, word ptr ss:DrawBottom
	mov     bp, ax
	shl     bp, 1
	add     bp, word ptr ss:ViewRows
	mov     ch, ss:YFraction
	mov     cl, byte ptr ss:FrameHeight
@@nextRow:
	add     ch, dh
	jb      @@drawRow
	inc     word ptr ss:SpanRow
	dec     cl
	jne     @@nextRow
	DRAW_EXIT
@@drawRow:
	mov     di, word ptr ss:SpanRow
@@nextSpan:
	mov     ax, [si].span_length
	or      ax, ax
	je      @@done
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
	call    word ptr ss:SpanProc
	jmp     @@drawRow
@@rowDone:
	inc     word ptr ss:SpanRow
	sub     bp, 2
	dec     cl
	je      @@done
	jmp     @@nextRow
@@done:
	DRAW_EXIT
RowsUp ENDP

; As RowsUp, skipping rows below the clip box and stopping above it.
ClippedRowsUp PROC FAR
	mov     ax, word ptr ss:DrawBottom
	mov     word ptr ss:ScreenY, ax
	mov     bp, ax
	shl     bp, 1
	add     bp, word ptr ss:ViewRows
	mov     ch, ss:YFraction
	mov     cl, byte ptr ss:FrameHeight
	cmp     ax, word ptr ss:ClipBottom
	jle     @@visible
@@below:
	inc     word ptr ss:SpanRow
	add     ch, dh
	jae     @@hidden
	sub     bp, 2
	dec     ax
	mov     word ptr ss:ScreenY, ax
	cmp     ax, word ptr ss:ClipBottom
	jle     @@visible
@@hidden:
	dec     cl
	jne     @@below
	je      @@done
@@visible:
	add     ch, dh
	jb      @@drawRow
	inc     word ptr ss:SpanRow
	dec     cl
	jne     @@visible
@@done:
	DRAW_EXIT
@@drawRow:
	mov     di, word ptr ss:SpanRow
@@nextSpan:
	mov     ax, [si].span_length
	or      ax, ax
	je      @@done
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
	call    word ptr ss:SpanProc
	jmp     @@drawRow
@@rowDone:
	inc     word ptr ss:SpanRow
	sub     bp, 2
	dec     word ptr ss:ScreenY
	mov     ax, word ptr ss:ScreenY
	cmp     ax, word ptr ss:ClipTop
	jl      @@above
	dec     cl
	jne     @@visible
@@above:
	DRAW_EXIT
ClippedRowsUp ENDP

; Draw one span into the screen row at ss:[bp], left to right from bx.
; ax is the span's length word. dl carries the column fraction: a pixel is
; stored only when adding the scale carries.
PlainSpan PROC NEAR
	push    ax
	mov     ax, [si].span_x
	add     ax, word ptr ss:FrameLeft
	mul     dh
	add     al, ss:XFraction
	mov     dl, al
	adc     ah, 0
	mov     al, ah
	xor     ah, ah
	add     ax, bx
	mov     di, [bp]
	add     di, ax
	add     si, SIZE SPANHDR
	pop     ax
	shr     ax, 1
	jb      @@runs
	mov     ah, al
	inc     ah
@@skip:
	dec     ah
	je      @@done
@@pixel:
	lodsb
	add     dl, dh
	jae     @@skip
	stosb
	dec     ah
	jne     @@pixel
@@done:
	ret
@@runs:
	mov     bx, ax
@@nextRun:
	lodsb
	shr     al, 1
	jb      @@fill
	sub     bl, al
	mov     ah, al
@@copy:
	lodsb
	add     dl, dh
	jae     @@copied
	stosb
@@copied:
	dec     ah
	jne     @@copy
	or      bl, bl
	jne     @@nextRun
	jmp     short @@spanDone
@@fill:
	sub     bl, al
	mov     ah, al
	lodsb
@@fillPixel:
	add     dl, dh
	jae     @@filled
	stosb
@@filled:
	dec     ah
	jne     @@fillPixel
	or      bl, bl
	jne     @@nextRun
@@spanDone:
	mov     bx, word ptr ss:ScreenX
	ret
PlainSpan ENDP

; As PlainSpan, right to left from bx.
PlainSpanMirror PROC NEAR
	push    ax
	mov     ax, [si].span_x
	add     ax, word ptr ss:FrameLeft
	mul     dh
	add     al, ss:XFraction
	mov     dl, al
	adc     ah, 0
	mov     al, ah
	xor     ah, ah
	sub     ax, bx
	neg     ax
	mov     di, [bp]
	add     di, ax
	add     si, SIZE SPANHDR
	pop     ax
	shr     ax, 1
	jb      @@runs
	mov     ah, al
	inc     ah
@@skip:
	dec     ah
	je      @@done
@@pixel:
	lodsb
	add     dl, dh
	jae     @@skip
	mov     es:[di], al
	dec     di
	dec     ah
	jne     @@pixel
@@done:
	ret
@@runs:
	mov     bx, ax
@@nextRun:
	lodsb
	shr     al, 1
	jb      @@fill
	sub     bl, al
	mov     ah, al
@@copy:
	lodsb
	add     dl, dh
	jae     @@copied
	mov     es:[di], al
	dec     di
@@copied:
	dec     ah
	jne     @@copy
	or      bl, bl
	jne     @@nextRun
	jmp     short @@spanDone
@@fill:
	sub     bl, al
	mov     ah, al
	lodsb
@@fillPixel:
	add     dl, dh
	jae     @@filled
	mov     es:[di], al
	dec     di
@@filled:
	dec     ah
	jne     @@fillPixel
	or      bl, bl
	jne     @@nextRun
@@spanDone:
	mov     bx, word ptr ss:DrawRight
	ret
PlainSpanMirror ENDP

; Draw one span clipped to the clip box's left and right. A run-encoded span
; is first unpacked into SpanBuffer.
ClipSpan PROC NEAR
	shr     ax, 1
	mov     word ptr ss:RunLength, ax
	jae     @@raw
	mov     bx, ax
	mov     ax, [si].span_x
	add     si, SIZE SPANHDR
	push    cx
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
	pop     cx
	mov     bx, word ptr ss:ScreenX
	push    si
	push    ds
	push    ss
	pop     ds
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
	add     ax, word ptr ss:FrameLeft
	mul     dh
	add     al, ss:XFraction
	mov     dl, al
	adc     ah, 0
	mov     al, ah
	xor     ah, ah
	add     ax, word ptr ss:ScreenX
	mov     bx, ax
	cmp     ax, word ptr ss:ClipRight
	jle     @@startsLeft
	jmp     @@done
@@startsLeft:
	mov     ax, word ptr ss:RunLength
	mul     dh
	add     al, dl
	adc     ah, 0
	mov     al, ah
	xor     ah, ah
	add     ax, bx
	cmp     ax, word ptr ss:ClipLeft
	jl      @@done
	cmp     ax, word ptr ss:ClipRight
	jle     @@rightIn
; count only the pixels up to the right edge
	mov     ax, word ptr ss:ClipRight
	inc     ax
	sub     ax, bx
	mov     ah, al
	xor     al, al
	sub     al, dl
	sbb     ah, 0
	jb      @@done
	div     dh
	or      ah, ah
	je      @@rightCount
	inc     al
@@rightCount:
	mov     byte ptr ss:RunLength, al
@@rightIn:
	mov     ax, bx
	sub     ax, word ptr ss:ClipLeft
	jge     @@leftIn
; skip the pixels left of the left edge
	or      ax, ax
	jns     @@leftCount
	neg     ax
@@leftCount:
	mov     ah, al
	xor     al, al
	sub     al, dl
	sbb     ah, 0
	jb      @@done
	div     dh
	or      ah, ah
	je      @@leftSkip
	inc     al
@@leftSkip:
	xor     ah, ah
	sub     word ptr ss:RunLength, ax
	jbe     @@done
	add     si, ax
	mul     dh
	add     dl, al
	adc     ah, 0
	mov     al, ah
	xor     ah, ah
	add     bx, ax
@@leftIn:
	mov     di, [bp]
	add     di, bx
	mov     ah, byte ptr ss:RunLength
@@pixel:
	lodsb
	add     dl, dh
	jae     @@skip
	stosb
@@skip:
	dec     ah
	jne     @@pixel
@@done:
	pop     ds
	pop     si
	ret
ClipSpan ENDP

; As ClipSpan, right to left from DrawRight.
ClipSpanMirror PROC NEAR
	shr     ax, 1
	mov     word ptr ss:RunLength, ax
	jae     @@raw
	mov     bx, ax
	mov     ax, [si].span_x
	add     si, SIZE SPANHDR
	push    cx
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
	pop     cx
	push    si
	push    ds
	push    ss
	pop     ds
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
	add     ax, word ptr ss:FrameLeft
	mul     dh
	add     al, ss:XFraction
	mov     dl, al
	adc     ah, 0
	mov     al, ah
	xor     ah, ah
	mov     bx, word ptr ss:DrawRight
	sub     bx, ax
	cmp     bx, word ptr ss:ClipLeft
	jge     @@startsRight
	jmp     @@done
@@startsRight:
	mov     ax, word ptr ss:RunLength
	mul     dh
	add     al, dl
	adc     ah, 0
	mov     al, ah
	xor     ah, ah
	sub     ax, bx
	neg     ax
	cmp     ax, word ptr ss:ClipRight
	jg      @@done
	cmp     ax, word ptr ss:ClipLeft
	jge     @@leftIn
; count only the pixels down to the left edge
	mov     ax, bx
	inc     ax
	sub     ax, word ptr ss:ClipLeft
	mov     ah, al
	xor     al, al
	sub     al, dl
	sbb     ah, 0
	jb      @@done
	div     dh
	or      ah, ah
	je      @@leftCount
	inc     al
@@leftCount:
	mov     byte ptr ss:RunLength, al
@@leftIn:
	mov     ax, bx
	sub     ax, word ptr ss:ClipRight
	jle     @@rightIn
; skip the pixels right of the right edge
	mov     ah, al
	xor     al, al
	sub     al, dl
	sbb     ah, 0
	jb      @@done
	div     dh
	or      ah, ah
	je      @@rightSkip
	inc     al
@@rightSkip:
	xor     ah, ah
	sub     word ptr ss:RunLength, ax
	jbe     @@done
	add     si, ax
	mul     dh
	add     dl, al
	adc     ah, 0
	mov     al, ah
	xor     ah, ah
	sub     bx, ax
@@rightIn:
	mov     di, [bp]
	add     di, bx
	mov     ah, byte ptr ss:RunLength
@@pixel:
	lodsb
	add     dl, dh
	jae     @@skip
	mov     es:[di], al
	dec     di
@@skip:
	dec     ah
	jne     @@pixel
@@done:
	pop     ds
	pop     si
	ret
ClipSpanMirror ENDP

	END
