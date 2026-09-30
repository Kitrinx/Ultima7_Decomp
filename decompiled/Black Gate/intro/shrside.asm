; Black Gate INTRO.EXE, resident segment 78 (file offsets 0x0194d0 to 0x019ad2, 1538 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

; Draws a shape frame shrunk (scale below 100h) and turned 90 degrees, in each
; of the four flips. Each row of the frame becomes a column on the view.

	.MODEL  MEDIUM
	LOCALS

	INCLUDE scalfram.inc

	.CODE

; Each entry places the frame and picks a span drawer by whether the frame
; needs clipping. Spans run down the view unless mirrored; rows go right to
; left unless flipped.
ShrinkSideways PROC FAR
	call    PlaceSideways
	jb      @@offView
	mov     bx, ss:DrawRight
	call    FrameNeedsClip
	jae     @@inside
	mov     ss:SpanProc, offset ClipSpanDown
	jmp     ClippedLeftwardColumns
@@inside:
	mov     ss:SpanProc, offset PlainSpanDown
	jmp     LeftwardColumns
@@offView:
	DRAW_EXIT
ShrinkSideways ENDP

ShrinkSidewaysMirror PROC FAR
	call    PlaceSidewaysMirror
	jb      @@offView
	mov     bx, ss:DrawRight
	mov     bp, ss:DrawBottom
	call    FrameNeedsClip
	jae     @@inside
	mov     ss:SpanProc, offset ClipSpanUp
	jmp     ClippedLeftwardColumns
@@inside:
	mov     ss:SpanProc, offset PlainSpanUp
	jmp     LeftwardColumns
@@offView:
	DRAW_EXIT
ShrinkSidewaysMirror ENDP

ShrinkSidewaysFlip PROC FAR
	call    PlaceSidewaysFlip
	jb      @@offView
	mov     bx, ss:DrawLeft
	call    FrameNeedsClip
	jae     @@inside
	mov     ss:SpanProc, offset ClipSpanDown
	jmp     ClippedRightwardColumns
@@inside:
	mov     ss:SpanProc, offset PlainSpanDown
	jmp     RightwardColumns
@@offView:
	DRAW_EXIT
ShrinkSidewaysFlip ENDP

ShrinkSidewaysBoth PROC FAR
	call    PlaceSidewaysBoth
	jb      @@offView
	mov     bx, ss:DrawLeft
	mov     bp, ss:DrawBottom
	call    FrameNeedsClip
	jae     @@inside
	mov     ss:SpanProc, offset ClipSpanUp
	jmp     ClippedRightwardColumns
@@inside:
	mov     ss:SpanProc, offset PlainSpanUp
	jmp     RightwardColumns
@@offView:
	DRAW_EXIT
ShrinkSidewaysBoth ENDP

; Walks the frame's rows, each drawn by SpanProc as a column, from bx leftward.
; dh is the scale: a row reaches the view only when adding it to the fraction
; in ch carries. ScreenY holds the top row's place in the row table.
LeftwardColumns PROC FAR
	shl     bp, 1
	add     bp, ss:ViewRows
	mov     ss:ScreenY, bp
	mov     ch, ss:XFraction
	mov     cl, byte ptr ss:FrameHeight
	mov     bp, ss:RowPitch
@@nextRow:
	add     ch, dh
	jb      @@drawRow
	inc     ss:SpanRow
	dec     cl
	jne     @@nextRow
	DRAW_EXIT
@@drawRow:
	mov     di, ss:SpanRow
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
	call    ss:SpanProc
	jmp     @@drawRow
@@rowDone:
	inc     ss:SpanRow
	dec     bx
	dec     cl
	jne     @@nextRow
@@done:
	DRAW_EXIT
LeftwardColumns ENDP

; LeftwardColumns from bx rightward.
RightwardColumns PROC FAR
	shl     bp, 1
	add     bp, ss:ViewRows
	mov     ss:ScreenY, bp
	mov     ch, ss:XFraction
	mov     cl, byte ptr ss:FrameHeight
	mov     bp, ss:RowPitch
@@nextRow:
	add     ch, dh
	jb      @@drawRow
	inc     ss:SpanRow
	dec     cl
	jne     @@nextRow
	DRAW_EXIT
@@drawRow:
	mov     di, ss:SpanRow
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
	call    ss:SpanProc
	jmp     @@drawRow
@@rowDone:
	inc     ss:SpanRow
	inc     bx
	dec     cl
	jne     @@nextRow
@@done:
	DRAW_EXIT
RightwardColumns ENDP

; Draws a span downward from its place in the column, one pixel for each carry
; of the scale into dl. bp is the row pitch; a run-encoded span holds runs of
; copied and repeated pixels.
PlainSpanDown PROC NEAR
	push    ax
	mov     ax, [si].span_x
	add     ax, ss:FrameLeft
	mul     dh
	add     al, ss:YFraction
	mov     dl, al
	adc     ah, 0
	mov     al, ah
	xor     ah, ah
	shl     ax, 1
	add     ax, ss:ScreenY
	mov     di, ax
	mov     di, ss:[di]
	add     di, bx
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
	mov     byte ptr es:[di], al
	add     di, bp
	dec     ah
	jne     @@pixel
@@done:
	ret
@@runs:
	push    bx
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
	mov     byte ptr es:[di], al
	add     di, bp
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
	mov     byte ptr es:[di], al
	add     di, bp
@@filled:
	dec     ah
	jne     @@fillPixel
	or      bl, bl
	jne     @@nextRun
@@spanDone:
	pop     bx
	ret
PlainSpanDown ENDP

; PlainSpanDown clipped to the view. A run-encoded span is unpacked into
; SpanBuffer first.
ClipSpanDown PROC NEAR
	shr     ax, 1
	mov     ss:RunLength, ax
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
	rep movsw
	rcl     cx, 1
	rep movsb
	sub     bx, ax
	jne     @@nextRun
	je      @@unpacked
@@fill:
	sub     bx, ax
	lodsb
	mov     ah, al
	shr     cx, 1
	rep stosw
	rcl     cx, 1
	rep stosb
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
	add     ax, ss:FrameLeft
	mul     dh
	add     al, ss:YFraction
	mov     dl, al
	adc     ah, 0
	mov     al, ah
	xor     ah, ah
	add     ax, ss:ScreenY
	mov     bx, ax
	cmp     ax, ss:ClipBottom
	jle     @@startsAbove
@@outside:
	jmp     @@done
@@startsAbove:
	mov     ax, ss:RunLength
	mul     dh
	add     al, dl
	adc     ah, 0
	mov     al, ah
	xor     ah, ah
	add     ax, bx
	cmp     ax, ss:ClipTop
	jl      @@outside
	cmp     ax, ss:ClipBottom
	jle     @@bottomIn
	mov     ax, ss:ClipBottom
	sub     ax, bx
	mov     ah, al
	xor     al, al
	inc     ah
	sub     al, dl
	sbb     ah, 0
	jb      @@done
	div     dh
	or      ah, ah
	je      @@bottomCount
	inc     al
@@bottomCount:
	mov     byte ptr ss:RunLength, al
@@bottomIn:
	mov     ax, bx
	sub     ax, ss:ClipTop
	jge     @@topIn
	or      ax, ax
	jns     @@topCount
	neg     ax
@@topCount:
	mov     ah, al
	xor     al, al
	sub     al, dl
	sbb     ah, 0
	jb      @@done
	div     dh
	or      ah, ah
	je      @@topSkip
	inc     al
@@topSkip:
	xor     ah, ah
	sub     ss:RunLength, ax
	jle     @@done
	add     si, ax
	mul     dh
	add     dl, al
	adc     ah, 0
	mov     al, ah
	xor     ah, ah
	add     bx, ax
@@topIn:
	mov     di, bx
	shl     di, 1
	add     di, ss:ViewRows
	mov     di, ss:[di]
	add     di, bp
	mov     bx, ss:RowPitch
	mov     ah, byte ptr ss:RunLength
@@pixel:
	lodsb
	add     dl, dh
	jae     @@skip
	mov     byte ptr es:[di], al
	add     di, bx
@@skip:
	dec     ah
	jne     @@pixel
@@done:
	pop     ds
	pop     si
	ret
ClipSpanDown ENDP

; PlainSpanDown drawn upward.
PlainSpanUp PROC NEAR
	push    ax
	mov     ax, [si].span_x
	add     ax, ss:FrameLeft
	mul     dh
	add     al, ss:YFraction
	mov     dl, al
	adc     ah, 0
	mov     al, ah
	xor     ah, ah
	shl     ax, 1
	neg     ax
	add     ax, ss:ScreenY
	mov     di, ax
	mov     di, ss:[di]
	add     di, bx
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
	mov     byte ptr es:[di], al
	sub     di, bp
	dec     ah
	jne     @@pixel
@@done:
	ret
@@runs:
	push    bx
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
	mov     byte ptr es:[di], al
	sub     di, bp
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
	mov     byte ptr es:[di], al
	sub     di, bp
@@filled:
	dec     ah
	jne     @@fillPixel
	or      bl, bl
	jne     @@nextRun
@@spanDone:
	pop     bx
	ret
PlainSpanUp ENDP

; ClipSpanDown drawn upward.
ClipSpanUp PROC NEAR
	shr     ax, 1
	mov     ss:RunLength, ax
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
	rep movsw
	rcl     cx, 1
	rep movsb
	sub     bx, ax
	jne     @@nextRun
	je      @@unpacked
@@fill:
	sub     bx, ax
	lodsb
	mov     ah, al
	shr     cx, 1
	rep stosw
	rcl     cx, 1
	rep stosb
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
	add     ax, ss:FrameLeft
	mul     dh
	add     al, ss:YFraction
	mov     dl, al
	adc     ah, 0
	mov     al, ah
	xor     ah, ah
	mov     bx, ss:ScreenY
	sub     bx, ax
	mov     ax, bx
	cmp     ax, ss:ClipTop
	jge     @@startsBelow
@@outside:
	jmp     @@done
@@startsBelow:
	mov     ax, ss:RunLength
	mul     dh
	add     al, dl
	adc     ah, 0
	mov     al, ah
	xor     ah, ah
	sub     ax, bx
	neg     ax
	cmp     ax, ss:ClipBottom
	jg      @@outside
	cmp     ax, ss:ClipTop
	jge     @@topIn
	mov     ax, ss:ClipTop
	sub     ax, bx
	neg     ax
	mov     ah, al
	xor     al, al
	inc     ah
	sub     al, dl
	sbb     ah, 0
	jb      @@done
	div     dh
	or      ah, ah
	je      @@topCount
	inc     al
@@topCount:
	mov     byte ptr ss:RunLength, al
@@topIn:
	mov     ax, bx
	sub     ax, ss:ClipBottom
	jle     @@bottomIn
	mov     ah, al
	xor     al, al
	sub     al, dl
	sbb     ah, 0
	jb      @@done
	div     dh
	or      ah, ah
	je      @@bottomSkip
	inc     al
@@bottomSkip:
	xor     ah, ah
	sub     ss:RunLength, ax
	jle     @@done
	add     si, ax
	mul     dh
	add     dl, al
	adc     ah, 0
	mov     al, ah
	xor     ah, ah
	sub     bx, ax
@@bottomIn:
	mov     di, bx
	shl     di, 1
	add     di, ss:ViewRows
	mov     di, ss:[di]
	add     di, bp
	mov     bx, ss:RowPitch
	mov     ah, byte ptr ss:RunLength
@@pixel:
	lodsb
	add     dl, dh
	jae     @@skip
	mov     byte ptr es:[di], al
	sub     di, bx
@@skip:
	dec     ah
	jne     @@pixel
@@done:
	pop     ds
	pop     si
	ret
ClipSpanUp ENDP

; LeftwardColumns for a frame that crosses the clip box: the rows right of it
; are passed over, and the walk stops at its left edge.
ClippedLeftwardColumns PROC FAR
	mov     ss:ScreenY, bp
	mov     ch, ss:XFraction
	mov     cl, byte ptr ss:FrameHeight
	mov     bp, bx
	cmp     bp, ss:ClipRight
	jle     @@nextRow
@@offRight:
	inc     ss:SpanRow
	add     ch, dh
	jae     @@hidden
	dec     bp
	cmp     bp, ss:ClipRight
	jle     @@nextRow
@@hidden:
	dec     cl
	jne     @@offRight
	je      @@finished
@@nextRow:
	add     ch, dh
	jb      @@drawRow
	inc     ss:SpanRow
	dec     cl
	jne     @@nextRow
@@finished:
	DRAW_EXIT
@@drawRow:
	mov     di, ss:SpanRow
@@nextSpan:
	mov     ax, [si].span_length
	or      ax, ax
	je      @@finished
	cmp     [si].span_y, di
	je      @@draw
	jl      @@passed
	jmp     short @@rowDone
; a span of a row that shrank away
@@passed:
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
	jmp     @@drawRow
@@rowDone:
	dec     bp
	cmp     bp, ss:ClipLeft
	jl      @@done
	inc     ss:SpanRow
	dec     cl
	jne     @@nextRow
@@done:
	DRAW_EXIT
ClippedLeftwardColumns ENDP

; ClippedLeftwardColumns from bx rightward.
ClippedRightwardColumns PROC FAR
	mov     ss:ScreenY, bp
	mov     ch, ss:XFraction
	mov     cl, byte ptr ss:FrameHeight
	mov     bp, bx
	cmp     bp, ss:ClipLeft
	jge     @@nextRow
@@offLeft:
	inc     ss:SpanRow
	add     ch, dh
	jae     @@hidden
	inc     bp
	cmp     bp, ss:ClipLeft
	jge     @@nextRow
@@hidden:
	dec     cl
	jne     @@offLeft
	je      @@finished
@@nextRow:
	add     ch, dh
	jb      @@drawRow
	inc     ss:SpanRow
	dec     cl
	jne     @@nextRow
@@finished:
	DRAW_EXIT
@@drawRow:
	mov     di, ss:SpanRow
@@nextSpan:
	mov     ax, [si].span_length
	or      ax, ax
	je      @@finished
	cmp     [si].span_y, di
	je      @@draw
	jl      @@passed
	jmp     short @@rowDone
; a span of a row that shrank away
@@passed:
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
	jmp     @@drawRow
@@rowDone:
	inc     bp
	cmp     bp, ss:ClipRight
	jg      @@done
	inc     ss:SpanRow
	dec     cl
	jne     @@nextRow
@@done:
	DRAW_EXIT
ClippedRightwardColumns ENDP

	END
