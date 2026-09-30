; Black Gate INTRO.EXE, resident segment 73 (file offsets 0x0138b0 to 0x014ab6, 4614 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

; Draws a shape frame shrunk (scale below 100h) and turned by 1 to 89 or
; 91 to 179 degrees, in each of the four flips.

	.MODEL  MEDIUM
	LOCALS

	INCLUDE scalfram.inc

	.DATA

; Each span drawer's pixel loops: plain, then filling the gaps a turned
; row leaves; FillGaps picks one.
TurnedSpanLoops                   dw  TurnedSpanLoop, TurnedSpanFill
TurnedClipSpanLoops               dw  TurnedClipSpanLoop, TurnedClipSpanFill
TurnedSpanMirrorLoops             dw  TurnedSpanMirrorLoop, TurnedSpanMirrorFill
TurnedClipSpanMirrorLoops         dw  TurnedClipSpanMirrorLoop, TurnedClipSpanMirrorFill
TurnedBackSpanLoops               dw  TurnedBackSpanLoop, TurnedBackSpanFill
TurnedBackClipSpanLoops           dw  TurnedBackClipSpanLoop, TurnedBackClipSpanFill
TurnedBackSpanMirrorLoops         dw  TurnedBackSpanMirrorLoop, TurnedBackSpanMirrorFill
TurnedBackClipSpanMirrorLoops     dw  TurnedBackClipSpanMirrorLoop, TurnedBackClipSpanMirrorFill

	.CODE

; Turned 1 to 89 degrees. Each entry places the frame, picks a span drawer
; by whether the frame needs clipping, and walks its rows.
ShrinkTurned PROC FAR
	call    far ptr PlaceTurned
	jb      @@offView
	call    far ptr FrameNeedsClip
	jae     @@inside
	mov     word ptr ss:SpanProc, offset TurnedClipSpan
	jmp     TurnedRows
@@offView:
	DRAW_EXIT
@@inside:
	mov     word ptr ss:SpanProc, offset TurnedSpan
	jmp     TurnedRows
ShrinkTurned ENDP

ShrinkTurnedMirror PROC FAR
	call    far ptr PlaceTurnedMirror
	jb      @@offView
	call    far ptr FrameNeedsClip
	jae     @@inside
	mov     word ptr ss:SpanProc, offset TurnedClipSpanMirror
	jmp     TurnedRows
@@offView:
	DRAW_EXIT
@@inside:
	mov     word ptr ss:SpanProc, offset TurnedSpanMirror
ShrinkTurnedMirror ENDP

; Walk the frame's rows. Each span of the current row is drawn from the
; row's start (bx, bp); then the start steps along the turned column axis,
; a column when dl carries and a row when dh carries. A step both ways
; leaves holes, so FillGaps then picks the pixel loops that fill them.
TurnedRows PROC FAR
	xor     dx, dx
@@rowStart:
	mov     word ptr ss:StepFraction, dx
	mov     word ptr ss:ScreenX, bx
	mov     word ptr ss:ScreenY, bp
@@nextSpan:
	mov     ax, [si].span_length
	or      ax, ax
	jne     @@haveSpan
	jmp     @@done
@@haveSpan:
	mov     dx, [si].span_y
	cmp     dx, word ptr ss:SpanRow
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
	jmp     @@nextSpan
@@rowDone:
	mov     bx, word ptr ss:ScreenX
	mov     bp, word ptr ss:ScreenY
	mov     dx, word ptr ss:StepFraction
@@nextRow:
	inc     word ptr ss:SpanRow
	dec     word ptr ss:FrameHeight
	je      @@done
	mov     ax, PLAIN_LOOP
	add     dl, ch
	jae     @@sameColumn
	dec     bx
	cmp     cl, ch
	jl      @@checkRow
	mov     ax, FILL_LOOP
	jmp     short @@checkRow
@@sameColumn:
	add     dh, cl
	jb      @@stepRow
	jae     @@nextRow
@@checkRow:
	add     dh, cl
	jae     @@setFill
@@stepRow:
	inc     bp
	cmp     cl, ch
	jge     @@setFill
	mov     ax, FILL_LOOP
@@setFill:
	mov     word ptr ss:FillGaps, ax
	jmp     @@rowStart
@@done:
	DRAW_EXIT
TurnedRows ENDP

ShrinkTurnedFlip PROC FAR
	call    far ptr PlaceTurnedFlip
	jb      @@offView
	call    far ptr FrameNeedsClip
	jae     @@inside
	mov     word ptr ss:SpanProc, offset TurnedClipSpan
	jmp     TurnedRowsFlip
@@offView:
	DRAW_EXIT
@@inside:
	mov     word ptr ss:SpanProc, offset TurnedSpan
	jmp     TurnedRowsFlip
ShrinkTurnedFlip ENDP

ShrinkTurnedBoth PROC FAR
	call    far ptr PlaceTurnedBoth
	jb      @@offView
	call    far ptr FrameNeedsClip
	jae     @@inside
	mov     word ptr ss:SpanProc, offset TurnedClipSpanMirror
	jmp     TurnedRowsFlip
@@offView:
	DRAW_EXIT
@@inside:
	mov     word ptr ss:SpanProc, offset TurnedSpanMirror
ShrinkTurnedBoth ENDP

; As TurnedRows, for the flipped rows: the start steps the other way.
TurnedRowsFlip PROC FAR
	xor     dx, dx
	mov     word ptr ss:StepFraction, dx
@@rowStart:
	mov     word ptr ss:ScreenX, bx
	mov     word ptr ss:ScreenY, bp
@@nextSpan:
	mov     ax, [si].span_length
	or      ax, ax
	jne     @@haveSpan
	jmp     @@done
@@haveSpan:
	mov     dx, [si].span_y
	cmp     dx, word ptr ss:SpanRow
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
	jmp     @@nextSpan
@@rowDone:
	mov     bx, word ptr ss:ScreenX
	mov     bp, word ptr ss:ScreenY
	mov     dx, word ptr ss:StepFraction
@@nextRow:
	inc     word ptr ss:SpanRow
	dec     word ptr ss:FrameHeight
	je      @@done
	mov     ax, PLAIN_LOOP
	add     dl, ch
	jae     @@sameColumn
	inc     bx
	jmp     short @@checkRow
@@sameColumn:
	add     dh, cl
	jb      @@stepRow
	jae     @@nextRow
@@checkRow:
	add     dh, cl
	jae     @@stepped
@@stepRow:
	dec     bp
@@stepped:
	mov     word ptr ss:StepFraction, dx
	cmp     cl, ch
	jge     @@rowFirst
@@columnFirst:
	add     dl, ch
	jb      @@gaps
	add     dh, cl
	jb      @@setFill
	jmp     @@columnFirst
@@rowFirst:
	add     dh, cl
	jb      @@gaps
	add     dl, ch
	jb      @@setFill
	jmp     @@rowFirst
@@gaps:
	mov     ax, FILL_LOOP
@@setFill:
	mov     word ptr ss:FillGaps, ax
	jmp     @@rowStart
@@done:
	DRAW_EXIT
TurnedRowsFlip ENDP

; Draw one span along the turned row axis from the row start. A
; run-encoded span is unpacked into SpanBuffer first; that costs cx, so the
; scaled cosine and sine are reloaded.
TurnedSpan PROC NEAR
	mov     ax, [si].span_x
	add     ax, word ptr ss:FrameLeft
	mov     bx, word ptr ss:ScreenX
	mov     di, word ptr ss:ScreenY
	push    ax
	mul     cl
	mov     dl, al
	mov     al, ah
	xor     ah, ah
	add     bx, ax
	pop     ax
	mul     ch
	mov     dh, al
	mov     al, ah
	xor     ah, ah
	add     di, ax
	shl     di, 1
	add     di, word ptr ss:ViewRows
	add     bx, word ptr ss:[di]
	mov     ax, [si].span_length
	add     si, SIZE SPANHDR
	shr     ax, 1
	jae     @@raw
	push    ax
	push    bx
	mov     bx, ax
	push    es
	mov     ax, ss
	mov     es, ax
	mov     di, offset SpanBuffer
@@nextRun:
	lodsb
	shr     al, 1
	cbw
	mov     cx, ax
	jb      @@fillRun
	shr     cx, 1
	rep     movsw
	rcl     cx, 1
	rep     movsb
	sub     bx, ax
	jne     @@nextRun
	je      @@unpacked
@@fillRun:
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
	pop     bx
	pop     ax
	push    si
	push    ds
	push    ss
	pop     ds
	mov     si, offset SpanBuffer
	mov     cx, word ptr ss:ScaledCos
	jmp     short @@draw
@@raw:
	mov     di, si
	add     di, ax
	push    di
	push    ds
@@draw:
	mov     ah, al
	mov     di, word ptr ss:FillGaps
	add     di, offset TurnedSpanLoops
	call    word ptr ss:[di]
	pop     ds
	pop     si
	ret
TurnedSpan ENDP

; As TurnedSpan, clipped. Pixels outside the near edges are skipped, the
; edges the span crosses are noted in ClipEdges, and ColumnsLeft and
; RowsLeft count how far it may run.
TurnedClipSpan PROC NEAR
	shr     ax, 1
	mov     word ptr ss:RunLength, ax
	mov     ax, [si].span_x
	jae     @@raw
	push    ax
	add     si, SIZE SPANHDR
	mov     bx, word ptr ss:RunLength
	push    es
	mov     ax, ss
	mov     es, ax
	mov     di, offset SpanBuffer
@@nextRun:
	lodsb
	shr     al, 1
	cbw
	mov     cx, ax
	jb      @@fillRun
	shr     cx, 1
	rep     movsw
	rcl     cx, 1
	rep     movsb
	sub     bx, ax
	jne     @@nextRun
	je      @@unpacked
@@fillRun:
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
	mov     cx, word ptr ss:ScaledCos
	pop     ax
	push    si
	push    ds
	push    ss
	pop     ds
	mov     si, offset SpanBuffer
	jmp     short @@place
@@raw:
	add     si, SIZE SPANHDR
	mov     di, si
	add     di, word ptr ss:RunLength
	push    di
	push    ds
@@place:
	add     ax, word ptr ss:FrameLeft
	mov     byte ptr ss:ClipEdges, 0
	mov     bx, word ptr ss:ScreenX
	mov     bp, word ptr ss:ScreenY
	mov     di, ax
	mul     cl
	mov     byte ptr ss:XFraction, al
	mov     al, ah
	xor     ah, ah
	add     bx, ax
	mov     ax, di
	mul     ch
	mov     byte ptr ss:YFraction, al
	mov     al, ah
	xor     ah, ah
	add     bp, ax
	mov     ax, bx
	cmp     ax, word ptr ss:ClipRight
	jle     @@xNotPast
	jmp     @@done
@@xNotPast:
	sub     ax, word ptr ss:ClipLeft
	jge     @@xStartIn
	or      byte ptr ss:ClipEdges, CLIP_LEFT
	or      ax, ax
	jns     @@xCount
	neg     ax
@@xCount:
	xor     dx, dx
	mov     dl, ah
	mov     ah, al
	xor     al, al
	sub     al, byte ptr ss:XFraction
	sbb     ah, 0
	sbb     dx, 0
	div     word ptr ss:CosStep
	or      dx, dx
	je      @@xSkip
	inc     ax
@@xSkip:
	add     si, ax
	sub     word ptr ss:RunLength, ax
	jle     @@gone
	push    ax
	mul     cl
	add     byte ptr ss:XFraction, al
	adc     ah, 0
	mov     al, ah
	xor     ah, ah
	add     bx, ax
	pop     ax
	mul     ch
	add     byte ptr ss:YFraction, al
	adc     ah, 0
	mov     al, ah
	xor     ah, ah
	add     bp, ax
@@xStartIn:
	mov     ax, bp
	cmp     ax, word ptr ss:ClipBottom
	jle     @@yNotPast
@@gone:
	jmp     @@done
@@yNotPast:
	sub     ax, word ptr ss:ClipTop
	jge     @@yStartIn
	or      ax, ax
	jns     @@yCount
	neg     ax
@@yCount:
	xor     dx, dx
	mov     dl, ah
	mov     ah, al
	xor     al, al
	sub     al, byte ptr ss:YFraction
	sbb     ah, 0
	sbb     dx, 0
	div     word ptr ss:SinStep
	or      dx, dx
	je      @@ySkip
	inc     ax
@@ySkip:
	add     si, ax
	sub     word ptr ss:RunLength, ax
	jle     @@gone
	push    ax
	mul     ch
	add     byte ptr ss:YFraction, al
	adc     ah, 0
	mov     al, ah
	xor     ah, ah
	add     bp, ax
	pop     ax
	mul     cl
	add     byte ptr ss:XFraction, al
	adc     ah, 0
	mov     al, ah
	xor     ah, ah
	add     bx, ax
	or      byte ptr ss:ClipEdges, CLIP_TOP
@@yStartIn:
	mov     ax, word ptr ss:ClipRight
	inc     ax
	sub     ax, bx
	jle     @@allGone
	mov     word ptr ss:ColumnsLeft, ax
	mov     ax, word ptr ss:RunLength
	mul     cl
	add     al, byte ptr ss:XFraction
	adc     ah, 0
	mov     di, ax
	mov     al, ah
	xor     ah, ah
	add     ax, bx
	cmp     ax, word ptr ss:ClipLeft
	jge     @@xEndReaches
@@allGone:
	jmp     @@done
@@xEndReaches:
	sub     ax, word ptr ss:ClipRight
	jle     @@xEndIn
	or      byte ptr ss:ClipEdges, CLIP_RIGHT
@@xEndIn:
	mov     ax, word ptr ss:ClipBottom
	inc     ax
	sub     ax, bp
	jle     @@allGone
	mov     word ptr ss:RowsLeft, ax
	mov     ax, word ptr ss:RunLength
	mul     ch
	add     al, byte ptr ss:YFraction
	adc     ah, 0
	mov     di, ax
	mov     al, ah
	xor     ah, ah
	add     ax, bp
	cmp     ax, word ptr ss:ClipTop
	jge     @@yEndReaches
	jmp     @@done
@@yEndReaches:
	sub     ax, word ptr ss:ClipBottom
	jle     @@yEndIn
	or      byte ptr ss:ClipEdges, CLIP_BOTTOM
@@yEndIn:
	mov     ax, bp
	shl     bp, 1
	add     bp, word ptr ss:ViewRows
	add     bx, [bp]
	mov     bp, ax
	mov     ah, byte ptr ss:RunLength
	or      ah, ah
	je      @@done
	mov     dx, word ptr ss:XFraction
	mov     di, word ptr ss:FillGaps
	add     di, offset TurnedClipSpanLoops
	call    word ptr ss:[di]
@@done:
	pop     ds
	pop     si
	ret
TurnedClipSpan ENDP

; As TurnedSpan, for the mirrored rows.
TurnedSpanMirror PROC NEAR
	mov     ax, [si].span_x
	add     ax, word ptr ss:FrameLeft
	mov     bx, word ptr ss:ScreenX
	mov     di, word ptr ss:ScreenY
	push    ax
	mul     cl
	mov     dl, al
	mov     al, ah
	xor     ah, ah
	sub     bx, ax
	pop     ax
	mul     ch
	mov     dh, al
	mov     al, ah
	xor     ah, ah
	sub     di, ax
	shl     di, 1
	add     di, word ptr ss:ViewRows
	add     bx, word ptr ss:[di]
	mov     ax, [si].span_length
	add     si, SIZE SPANHDR
	shr     ax, 1
	jae     @@raw
	push    ax
	push    bx
	mov     bx, ax
	push    es
	mov     ax, ss
	mov     es, ax
	mov     di, offset SpanBuffer
@@nextRun:
	lodsb
	shr     al, 1
	cbw
	mov     cx, ax
	jb      @@fillRun
	shr     cx, 1
	rep     movsw
	rcl     cx, 1
	rep     movsb
	sub     bx, ax
	jne     @@nextRun
	je      @@unpacked
@@fillRun:
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
	pop     bx
	pop     ax
	push    si
	push    ds
	push    ss
	pop     ds
	mov     si, offset SpanBuffer
	mov     cx, word ptr ss:ScaledCos
	jmp     short @@draw
@@raw:
	mov     di, si
	add     di, ax
	push    di
	push    ds
@@draw:
	mov     ah, al
	mov     di, word ptr ss:FillGaps
	add     di, offset TurnedSpanMirrorLoops
	call    word ptr ss:[di]
	pop     ds
	pop     si
	ret
TurnedSpanMirror ENDP

; As TurnedClipSpan, for the mirrored rows.
TurnedClipSpanMirror PROC NEAR
	shr     ax, 1
	mov     word ptr ss:RunLength, ax
	mov     ax, [si].span_x
	jae     @@raw
	push    ax
	add     si, SIZE SPANHDR
	mov     bx, word ptr ss:RunLength
	push    es
	mov     ax, ss
	mov     es, ax
	mov     di, offset SpanBuffer
@@nextRun:
	lodsb
	shr     al, 1
	cbw
	mov     cx, ax
	jb      @@fillRun
	shr     cx, 1
	rep     movsw
	rcl     cx, 1
	rep     movsb
	sub     bx, ax
	jne     @@nextRun
	je      @@unpacked
@@fillRun:
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
	mov     cx, word ptr ss:ScaledCos
	pop     ax
	push    si
	push    ds
	push    ss
	pop     ds
	mov     si, offset SpanBuffer
	jmp     short @@place
@@raw:
	add     si, SIZE SPANHDR
	mov     di, si
	add     di, word ptr ss:RunLength
	push    di
	push    ds
@@place:
	add     ax, word ptr ss:FrameLeft
	mov     byte ptr ss:ClipEdges, 0
	mov     bx, word ptr ss:ScreenX
	mov     bp, word ptr ss:ScreenY
	mov     di, ax
	mul     cl
	mov     byte ptr ss:XFraction, al
	mov     al, ah
	xor     ah, ah
	sub     bx, ax
	mov     ax, di
	mul     ch
	mov     byte ptr ss:YFraction, al
	mov     al, ah
	xor     ah, ah
	sub     bp, ax
	mov     ax, bx
	cmp     ax, word ptr ss:ClipLeft
	jge     @@xNotPast
	jmp     @@done
@@xNotPast:
	sub     ax, word ptr ss:ClipRight
	jle     @@xStartIn
	or      byte ptr ss:ClipEdges, CLIP_RIGHT
	xor     dx, dx
	mov     dl, ah
	mov     ah, al
	xor     al, al
	sub     al, byte ptr ss:XFraction
	sbb     ah, 0
	sbb     dx, 0
	div     word ptr ss:CosStep
	or      dx, dx
	je      @@xSkip
	inc     ax
@@xSkip:
	add     si, ax
	sub     word ptr ss:RunLength, ax
	jle     @@gone
	push    ax
	mul     cl
	add     byte ptr ss:XFraction, al
	adc     ah, 0
	mov     al, ah
	xor     ah, ah
	sub     bx, ax
	pop     ax
	mul     ch
	add     byte ptr ss:YFraction, al
	adc     ah, 0
	mov     al, ah
	xor     ah, ah
	sub     bp, ax
@@xStartIn:
	mov     ax, bp
	cmp     ax, word ptr ss:ClipTop
	jge     @@yNotPast
@@gone:
	jmp     @@done
@@yNotPast:
	sub     ax, word ptr ss:ClipBottom
	jle     @@yStartIn
	xor     dx, dx
	mov     dl, ah
	mov     ah, al
	xor     al, al
	sub     al, byte ptr ss:YFraction
	sbb     ah, 0
	sbb     dx, 0
	div     word ptr ss:SinStep
	or      dx, dx
	je      @@ySkip
	inc     ax
@@ySkip:
	add     si, ax
	sub     word ptr ss:RunLength, ax
	jle     @@gone
	push    ax
	mul     ch
	add     byte ptr ss:YFraction, al
	adc     ah, 0
	mov     al, ah
	xor     ah, ah
	sub     bp, ax
	pop     ax
	mul     cl
	add     byte ptr ss:XFraction, al
	adc     ah, 0
	mov     al, ah
	xor     ah, ah
	sub     bx, ax
	or      byte ptr ss:ClipEdges, CLIP_BOTTOM
@@yStartIn:
	mov     ax, bx
	inc     ax
	sub     ax, word ptr ss:ClipLeft
	jle     @@allGone
	mov     word ptr ss:ColumnsLeft, ax
	mov     ax, word ptr ss:RunLength
	mul     cl
	add     al, byte ptr ss:XFraction
	adc     ah, 0
	mov     di, ax
	mov     al, ah
	xor     ah, ah
	sub     ax, bx
	neg     ax
	cmp     ax, word ptr ss:ClipRight
	jle     @@xEndReaches
@@allGone:
	jmp     @@done
@@xEndReaches:
	sub     ax, word ptr ss:ClipLeft
	jge     @@xEndIn
	or      byte ptr ss:ClipEdges, CLIP_LEFT
@@xEndIn:
	mov     ax, bp
	inc     ax
	sub     ax, word ptr ss:ClipTop
	jle     @@allGone
	mov     word ptr ss:RowsLeft, ax
	mov     ax, word ptr ss:RunLength
	mul     ch
	add     al, byte ptr ss:YFraction
	adc     ah, 0
	mov     di, ax
	mov     al, ah
	xor     ah, ah
	sub     ax, bp
	neg     ax
	cmp     ax, word ptr ss:ClipBottom
	jle     @@yEndReaches
	jmp     @@done
@@yEndReaches:
	sub     ax, word ptr ss:ClipTop
	jge     @@yEndIn
	or      byte ptr ss:ClipEdges, CLIP_TOP
@@yEndIn:
	mov     ax, bx
	shl     bp, 1
	add     bp, word ptr ss:ViewRows
	add     bx, [bp]
	mov     bp, ax
	mov     ah, byte ptr ss:RunLength
	or      ah, ah
	je      @@done
	mov     dx, word ptr ss:XFraction
	mov     di, word ptr ss:FillGaps
	add     di, offset TurnedClipSpanMirrorLoops
	call    word ptr ss:[di]
@@done:
	pop     ds
	pop     si
	ret
TurnedClipSpanMirror ENDP

; The span drawers for 91 to 179 degrees, as those above.
TurnedBackSpan PROC NEAR
	mov     ax, [si].span_x
	add     ax, word ptr ss:FrameLeft
	mov     bx, word ptr ss:ScreenX
	mov     bp, word ptr ss:ScreenY
	mov     di, ax
	mul     cl
	mov     dl, al
	mov     al, ah
	xor     ah, ah
	sub     bx, ax
	mov     ax, di
	mul     ch
	mov     dh, al
	mov     al, ah
	xor     ah, ah
	add     bp, ax
	shl     bp, 1
	add     bp, word ptr ss:ViewRows
	add     bx, [bp]
	mov     ax, [si].span_length
	add     si, SIZE SPANHDR
	shr     ax, 1
	jae     @@raw
	push    ax
	push    bx
	mov     bx, ax
	push    es
	mov     ax, ss
	mov     es, ax
	mov     di, offset SpanBuffer
@@nextRun:
	lodsb
	shr     al, 1
	cbw
	mov     cx, ax
	jb      @@fillRun
	shr     cx, 1
	rep     movsw
	rcl     cx, 1
	rep     movsb
	sub     bx, ax
	jne     @@nextRun
	je      @@unpacked
@@fillRun:
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
	pop     bx
	pop     ax
	push    si
	push    ds
	push    ss
	pop     ds
	mov     si, offset SpanBuffer
	mov     cx, word ptr ss:ScaledCos
	jmp     short @@draw
@@raw:
	mov     di, si
	add     di, ax
	push    di
	push    ds
@@draw:
	mov     ah, al
	mov     di, word ptr ss:FillGaps
	add     di, offset TurnedBackSpanLoops
	call    word ptr ss:[di]
	pop     ds
	pop     si
	ret
TurnedBackSpan ENDP

TurnedBackClipSpan PROC NEAR
	shr     ax, 1
	mov     word ptr ss:RunLength, ax
	mov     ax, [si].span_x
	jae     @@raw
	push    ax
	add     si, SIZE SPANHDR
	mov     bx, word ptr ss:RunLength
	push    es
	mov     ax, ss
	mov     es, ax
	mov     di, offset SpanBuffer
@@nextRun:
	lodsb
	shr     al, 1
	cbw
	mov     cx, ax
	jb      @@fillRun
	shr     cx, 1
	rep     movsw
	rcl     cx, 1
	rep     movsb
	sub     bx, ax
	jne     @@nextRun
	je      @@unpacked
@@fillRun:
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
	mov     cx, word ptr ss:ScaledCos
	pop     ax
	push    si
	push    ds
	push    ss
	pop     ds
	mov     si, offset SpanBuffer
	jmp     short @@place
@@raw:
	add     si, SIZE SPANHDR
	mov     di, si
	add     di, word ptr ss:RunLength
	push    di
	push    ds
@@place:
	add     ax, word ptr ss:FrameLeft
	mov     byte ptr ss:ClipEdges, 0
	mov     bx, word ptr ss:ScreenX
	mov     bp, word ptr ss:ScreenY
	mov     di, ax
	mul     cl
	mov     byte ptr ss:XFraction, al
	mov     al, ah
	xor     ah, ah
	sub     bx, ax
	mov     ax, di
	mul     ch
	mov     byte ptr ss:YFraction, al
	mov     al, ah
	xor     ah, ah
	add     bp, ax
	mov     ax, bx
	cmp     ax, word ptr ss:ClipLeft
	jge     @@xNotPast
	jmp     @@done
@@xNotPast:
	sub     ax, word ptr ss:ClipRight
	jle     @@xStartIn
	or      byte ptr ss:ClipEdges, CLIP_RIGHT
	xor     dx, dx
	mov     dl, ah
	mov     ah, al
	xor     al, al
	sub     al, byte ptr ss:XFraction
	sbb     ah, 0
	sbb     dx, 0
	div     word ptr ss:CosStep
	or      dx, dx
	je      @@xSkip
	inc     ax
@@xSkip:
	add     si, ax
	sub     word ptr ss:RunLength, ax
	jle     @@gone
	push    ax
	mul     cl
	add     byte ptr ss:XFraction, al
	adc     ah, 0
	mov     al, ah
	xor     ah, ah
	sub     bx, ax
	pop     ax
	mul     ch
	add     byte ptr ss:YFraction, al
	adc     ah, 0
	mov     al, ah
	xor     ah, ah
	add     bp, ax
@@xStartIn:
	mov     ax, bp
	cmp     ax, word ptr ss:ClipBottom
	jle     @@yNotPast
@@gone:
	jmp     @@done
@@yNotPast:
	sub     ax, word ptr ss:ClipTop
	jge     @@yStartIn
	or      ax, ax
	jns     @@yCount
	neg     ax
@@yCount:
	xor     dx, dx
	mov     dl, ah
	mov     ah, al
	xor     al, al
	sub     al, byte ptr ss:YFraction
	sbb     ah, 0
	sbb     dx, 0
	div     word ptr ss:SinStep
	or      dx, dx
	je      @@ySkip
	inc     ax
@@ySkip:
	add     si, ax
	sub     word ptr ss:RunLength, ax
	jle     @@gone
	push    ax
	mul     ch
	add     byte ptr ss:YFraction, al
	adc     ah, 0
	mov     al, ah
	xor     ah, ah
	add     bp, ax
	pop     ax
	mul     cl
	add     byte ptr ss:XFraction, al
	adc     ah, 0
	mov     al, ah
	xor     ah, ah
	sub     bx, ax
	or      byte ptr ss:ClipEdges, CLIP_TOP
@@yStartIn:
	mov     ax, bx
	inc     ax
	sub     ax, word ptr ss:ClipLeft
	jle     @@allGone
	mov     word ptr ss:ColumnsLeft, ax
	mov     ax, word ptr ss:RunLength
	mul     cl
	add     al, byte ptr ss:XFraction
	adc     ah, 0
	mov     di, ax
	mov     al, ah
	xor     ah, ah
	mov     dx, bx
	sub     dx, ax
	cmp     dx, word ptr ss:ClipRight
	jle     @@xEndReaches
@@allGone:
	jmp     @@done
@@xEndReaches:
	cmp     dx, word ptr ss:ClipLeft
	jge     @@xEndIn
	or      byte ptr ss:ClipEdges, CLIP_LEFT
@@xEndIn:
	mov     ax, word ptr ss:ClipBottom
	inc     ax
	sub     ax, bp
	jle     @@allGone
	mov     word ptr ss:RowsLeft, ax
	mov     ax, word ptr ss:RunLength
	mul     ch
	add     al, byte ptr ss:YFraction
	adc     ah, 0
	mov     di, ax
	mov     al, ah
	xor     ah, ah
	add     ax, bp
	cmp     ax, word ptr ss:ClipTop
	jge     @@yEndReaches
	jmp     @@done
@@yEndReaches:
	cmp     ax, word ptr ss:ClipBottom
	jle     @@yEndIn
	or      byte ptr ss:ClipEdges, CLIP_BOTTOM
@@yEndIn:
	mov     ax, bp
	shl     bp, 1
	add     bp, word ptr ss:ViewRows
	add     bx, [bp]
	mov     bp, ax
	mov     ah, byte ptr ss:RunLength
	or      ah, ah
	je      @@done
	mov     dx, word ptr ss:XFraction
	mov     di, word ptr ss:FillGaps
	add     di, offset TurnedBackClipSpanLoops
	call    word ptr ss:[di]
@@done:
	pop     ds
	pop     si
	ret
TurnedBackClipSpan ENDP

TurnedBackSpanMirror PROC NEAR
	mov     ax, [si].span_x
	add     ax, word ptr ss:FrameLeft
	mov     bx, word ptr ss:ScreenX
	mov     bp, word ptr ss:ScreenY
	mov     di, ax
	mul     cl
	mov     dl, al
	mov     al, ah
	xor     ah, ah
	add     bx, ax
	mov     ax, di
	mul     ch
	mov     dh, al
	mov     al, ah
	xor     ah, ah
	sub     bp, ax
	shl     bp, 1
	add     bp, word ptr ss:ViewRows
	add     bx, [bp]
	mov     ax, [si].span_length
	add     si, SIZE SPANHDR
	shr     ax, 1
	jae     @@raw
	push    ax
	push    bx
	mov     bx, ax
	push    es
	mov     ax, ss
	mov     es, ax
	mov     di, offset SpanBuffer
@@nextRun:
	lodsb
	shr     al, 1
	cbw
	mov     cx, ax
	jb      @@fillRun
	shr     cx, 1
	rep     movsw
	rcl     cx, 1
	rep     movsb
	sub     bx, ax
	jne     @@nextRun
	je      @@unpacked
@@fillRun:
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
	pop     bx
	pop     ax
	push    si
	push    ds
	push    ss
	pop     ds
	mov     si, offset SpanBuffer
	mov     cx, word ptr ss:ScaledCos
	jmp     short @@draw
@@raw:
	mov     di, si
	add     di, ax
	push    di
	push    ds
@@draw:
	mov     ah, al
	mov     di, word ptr ss:FillGaps
	add     di, offset TurnedBackSpanMirrorLoops
	call    word ptr ss:[di]
	pop     ds
	pop     si
	ret
TurnedBackSpanMirror ENDP

TurnedBackClipSpanMirror PROC NEAR
	shr     ax, 1
	mov     word ptr ss:RunLength, ax
	mov     ax, [si].span_x
	jae     @@raw
	push    ax
	add     si, SIZE SPANHDR
	mov     bx, word ptr ss:RunLength
	push    es
	mov     ax, ss
	mov     es, ax
	mov     di, offset SpanBuffer
@@nextRun:
	lodsb
	shr     al, 1
	cbw
	mov     cx, ax
	jb      @@fillRun
	shr     cx, 1
	rep     movsw
	rcl     cx, 1
	rep     movsb
	sub     bx, ax
	jne     @@nextRun
	je      @@unpacked
@@fillRun:
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
	mov     cx, word ptr ss:ScaledCos
	pop     ax
	push    si
	push    ds
	push    ss
	pop     ds
	mov     si, offset SpanBuffer
	jmp     short @@place
@@raw:
	add     si, SIZE SPANHDR
	mov     di, si
	add     di, word ptr ss:RunLength
	push    di
	push    ds
@@place:
	add     ax, word ptr ss:FrameLeft
	mov     byte ptr ss:ClipEdges, 0
	mov     bx, word ptr ss:ScreenX
	mov     bp, word ptr ss:ScreenY
	mov     di, ax
	mul     cl
	mov     byte ptr ss:XFraction, al
	mov     al, ah
	xor     ah, ah
	add     bx, ax
	mov     ax, di
	mul     ch
	mov     byte ptr ss:YFraction, al
	mov     al, ah
	xor     ah, ah
	sub     bp, ax
	mov     ax, bx
	cmp     ax, word ptr ss:ClipRight
	jle     @@xNotPast
	jmp     @@done
@@xNotPast:
	sub     ax, word ptr ss:ClipLeft
	jge     @@xStartIn
	neg     ax
	or      byte ptr ss:ClipEdges, CLIP_LEFT
	xor     dx, dx
	mov     dl, ah
	mov     ah, al
	xor     al, al
	sub     al, byte ptr ss:XFraction
	sbb     ah, 0
	sbb     dx, 0
	div     word ptr ss:CosStep
	or      dx, dx
	je      @@xSkip
	inc     ax
@@xSkip:
	add     si, ax
	sub     word ptr ss:RunLength, ax
	jle     @@gone
	push    ax
	mul     cl
	add     byte ptr ss:XFraction, al
	adc     ah, 0
	mov     al, ah
	xor     ah, ah
	add     bx, ax
	pop     ax
	mul     ch
	add     byte ptr ss:YFraction, al
	adc     ah, 0
	mov     al, ah
	xor     ah, ah
	sub     bp, ax
@@xStartIn:
	mov     ax, bp
	cmp     ax, word ptr ss:ClipTop
	jge     @@yNotPast
@@gone:
	jmp     @@done
@@yNotPast:
	sub     ax, word ptr ss:ClipBottom
	jle     @@yStartIn
	xor     dx, dx
	mov     dl, ah
	mov     ah, al
	xor     al, al
	sub     al, byte ptr ss:YFraction
	sbb     ah, 0
	sbb     dx, 0
	div     word ptr ss:SinStep
	or      dx, dx
	je      @@ySkip
	inc     ax
@@ySkip:
	add     si, ax
	sub     word ptr ss:RunLength, ax
	jle     @@gone
	push    ax
	mul     ch
	add     byte ptr ss:YFraction, al
	adc     ah, 0
	mov     al, ah
	xor     ah, ah
	sub     bp, ax
	pop     ax
	mul     cl
	add     byte ptr ss:XFraction, al
	adc     ah, 0
	mov     al, ah
	xor     ah, ah
	add     bx, ax
	or      byte ptr ss:ClipEdges, CLIP_BOTTOM
@@yStartIn:
	mov     ax, word ptr ss:ClipRight
	inc     ax
	sub     ax, bx
	jle     @@allGone
	mov     word ptr ss:ColumnsLeft, ax
	mov     ax, word ptr ss:RunLength
	mul     cl
	add     al, byte ptr ss:XFraction
	adc     ah, 0
	mov     di, ax
	mov     al, ah
	xor     ah, ah
	add     ax, bx
	cmp     ax, word ptr ss:ClipLeft
	jge     @@xEndReaches
@@allGone:
	jmp     @@done
@@xEndReaches:
	cmp     ax, word ptr ss:ClipRight
	jle     @@xEndIn
	or      byte ptr ss:ClipEdges, CLIP_RIGHT
@@xEndIn:
	mov     ax, bp
	inc     ax
	sub     ax, word ptr ss:ClipTop
	jle     @@allGone
	mov     word ptr ss:RowsLeft, ax
	mov     ax, word ptr ss:RunLength
	mul     ch
	add     al, byte ptr ss:YFraction
	adc     ah, 0
	mov     al, ah
	xor     ah, ah
	neg     ax
	add     ax, bp
	cmp     ax, word ptr ss:ClipBottom
	jle     @@yEndReaches
	jmp     @@done
@@yEndReaches:
	cmp     ax, word ptr ss:ClipTop
	jge     @@yEndIn
	or      byte ptr ss:ClipEdges, CLIP_TOP
@@yEndIn:
	mov     ax, bx
	shl     bp, 1
	add     bp, word ptr ss:ViewRows
	add     bx, [bp]
	mov     bp, ax
	mov     ah, byte ptr ss:RunLength
	or      ah, ah
	je      @@done
	mov     dx, word ptr ss:XFraction
	mov     di, word ptr ss:FillGaps
	add     di, offset TurnedBackClipSpanMirrorLoops
	call    word ptr ss:[di]
@@done:
	pop     ds
	pop     si
	ret
TurnedBackClipSpanMirror ENDP

; Store ah pixels from ds:si at es:[bx] along the turned row axis, di
; being the screen's row pitch. A source pixel reaches the screen only
; when the position moves.
TurnedSpanLoop PROC NEAR
	mov     di, word ptr ss:RowPitch
	jmp     @@store
@@next:
	add     dl, cl
	jb      @@stepColumn
	add     dh, ch
	jb      @@stepRow
	inc     si
	dec     ah
	jne     @@next
	ret
@@stepColumn:
	inc     bx
	add     dh, ch
	jae     @@store
@@stepRow:
	add     bx, di
@@store:
	lodsb
	mov     byte ptr es:[bx], al
	dec     ah
	jne     @@next
	ret
TurnedSpanLoop ENDP

; As TurnedSpanLoop, stopping at the far clip edges; the plain loop
; serves when the span crosses neither.
TurnedClipSpanLoop PROC NEAR
	test    byte ptr ss:ClipEdges, CLIP_RIGHT OR CLIP_BOTTOM
	je      TurnedSpanLoop
	mov     di, word ptr ss:RowPitch
	jmp     @@store
@@next:
	add     dl, cl
	jb      @@stepColumn
	add     dh, ch
	jb      @@stepRow
	inc     si
	dec     ah
	jne     @@next
	ret
@@stepColumn:
	dec     word ptr ss:ColumnsLeft
	jle     @@done
	inc     bx
	add     dh, ch
	jae     @@store
@@stepRow:
	dec     word ptr ss:RowsLeft
	jle     @@done
	add     bx, di
@@store:
	lodsb
	mov     byte ptr es:[bx], al
	dec     ah
	jne     @@next
@@done:
	ret
TurnedClipSpanLoop ENDP

; As TurnedSpanLoop, also storing a pixel in the corner of each
; diagonal step, so a turned frame shows no holes.
TurnedSpanFill PROC NEAR
	mov     di, word ptr ss:RowPitch
	jmp     @@store
@@next:
	add     dl, cl
	jb      @@stepColumn
	add     dh, ch
	jb      @@stepRow
	lodsb
	dec     ah
	jne     @@next
	ret
@@stepColumn:
	inc     bx
	add     dh, ch
	jae     @@store
	mov     byte ptr es:[bx], al
@@stepRow:
	add     bx, di
@@store:
	lodsb
	mov     byte ptr es:[bx], al
	dec     ah
	jne     @@next
	ret
TurnedSpanFill ENDP

; As TurnedSpanFill, clipped. When the span's start was clipped,
; the corner pixel before its first visible one is filled first.
TurnedClipSpanFill PROC NEAR
	mov     di, word ptr ss:RowPitch
	test    byte ptr ss:ClipEdges, CLIP_LEFT
	jne     @@clippedStart
	test    byte ptr ss:ClipEdges, CLIP_RIGHT OR CLIP_BOTTOM
	je      TurnedSpanFill
	mov     ah, byte ptr ss:RunLength
	jmp     @@store
@@clippedStart:
	cmp     bp, word ptr ss:ClipTop
	je      @@store
	push    si
	push    dx
@@back:
	dec     si
	sub     dl, cl
	jae     @@backRow
	sub     dh, ch
	jae     @@backDone
	mov     al, byte ptr [si]
	sub     bx, di
	mov     byte ptr es:[bx], al
	add     bx, di
	jmp     short @@backDone
@@backRow:
	sub     dh, ch
	jae     @@back
@@backDone:
	pop     dx
	pop     si
	jmp     @@store
@@next:
	add     dl, cl
	jb      @@stepColumn
	add     dh, ch
	jb      @@stepRow
	lodsb
	dec     ah
	jne     @@next
	ret
@@stepColumn:
	dec     word ptr ss:ColumnsLeft
	jle     @@done
	inc     bx
	add     dh, ch
	jae     @@store
	mov     byte ptr es:[bx], al
@@stepRow:
	add     bx, di
	dec     word ptr ss:RowsLeft
	jle     @@done
@@store:
	lodsb
	mov     byte ptr es:[bx], al
	dec     ah
	jne     @@next
@@done:
	ret
TurnedClipSpanFill ENDP

; The pixel loops of the other span drawers, as those above.
TurnedBackSpanMirrorLoop PROC NEAR
	mov     di, word ptr ss:RowPitch
	jmp     @@store
@@next:
	add     dl, cl
	jb      @@stepColumn
	add     dh, ch
	jb      @@stepRow
	inc     si
	dec     ah
	jne     @@next
	ret
@@stepColumn:
	inc     bx
	add     dh, ch
	jae     @@store
@@stepRow:
	sub     bx, di
@@store:
	lodsb
	mov     byte ptr es:[bx], al
	dec     ah
	jne     @@next
	ret
TurnedBackSpanMirrorLoop ENDP

TurnedBackClipSpanMirrorLoop PROC NEAR
	test    byte ptr ss:ClipEdges, CLIP_TOP OR CLIP_RIGHT
	je      TurnedBackSpanMirrorLoop
	mov     di, word ptr ss:RowPitch
	jmp     short @@store
@@next:
	add     dl, cl
	jb      @@stepColumn
	add     dh, ch
	jb      @@stepRow
	inc     si
	dec     ah
	jne     @@next
	ret
@@stepColumn:
	dec     word ptr ss:ColumnsLeft
	jle     @@done
	inc     bx
	add     dh, ch
	jae     @@store
@@stepRow:
	dec     word ptr ss:RowsLeft
	jle     @@done
	sub     bx, di
@@store:
	lodsb
	mov     byte ptr es:[bx], al
	dec     ah
	jne     @@next
@@done:
	ret
TurnedBackClipSpanMirrorLoop ENDP

TurnedBackSpanMirrorFill PROC NEAR
	mov     di, word ptr ss:RowPitch
	jmp     short @@store
@@next:
	add     dl, cl
	jb      @@stepColumn
	add     dh, ch
	jb      @@stepRow
	lodsb
	dec     ah
	jne     @@next
	ret
@@stepColumn:
	add     dh, ch
	jae     @@moveColumn
	sub     bx, di
	mov     byte ptr es:[bx], al
@@moveColumn:
	inc     bx
@@store:
	lodsb
	mov     byte ptr es:[bx], al
	dec     ah
	jne     @@next
	ret
@@stepRow:
	sub     bx, di
	lodsb
	mov     byte ptr es:[bx], al
	dec     ah
	jne     @@next
	ret
TurnedBackSpanMirrorFill ENDP

TurnedBackClipSpanMirrorFill PROC NEAR
	mov     di, word ptr ss:RowPitch
	test    byte ptr ss:ClipEdges, CLIP_BOTTOM
	jne     @@clippedStart
	test    byte ptr ss:ClipEdges, CLIP_TOP OR CLIP_RIGHT
	je      TurnedBackSpanMirrorFill
	jmp     short @@store
@@clippedStart:
	cmp     bp, word ptr ss:ClipLeft
	je      @@store
	push    si
	push    dx
@@back:
	dec     si
	sub     dl, cl
	jae     @@backRow
	sub     dh, ch
	jae     @@backDone
	mov     al, byte ptr [si]
	dec     bx
	mov     byte ptr es:[bx], al
	inc     bx
	jmp     short @@backDone
@@backRow:
	sub     dh, ch
	jae     @@back
@@backDone:
	pop     dx
	pop     si
	jmp     @@store
@@next:
	add     dl, cl
	jb      @@stepColumn
	add     dh, ch
	jb      @@stepRow
	lodsb
	dec     ah
	jne     @@next
	ret
@@stepColumn:
	add     dh, ch
	jae     @@moveColumn
	dec     word ptr ss:RowsLeft
	jle     @@done
	sub     bx, di
	mov     byte ptr es:[bx], al
@@moveColumn:
	dec     word ptr ss:ColumnsLeft
	jle     @@done
	inc     bx
	lodsb
	mov     byte ptr es:[bx], al
	dec     ah
	jne     @@next
	ret
@@stepRow:
	sub     bx, di
	dec     word ptr ss:RowsLeft
	jle     @@done
@@store:
	lodsb
	mov     byte ptr es:[bx], al
	dec     ah
	jne     @@next
@@done:
	ret
TurnedBackClipSpanMirrorFill ENDP

TurnedBackSpanLoop PROC NEAR
	mov     di, word ptr ss:RowPitch
	jmp     @@store
@@next:
	add     dl, cl
	jb      @@stepColumn
	add     dh, ch
	jb      @@stepRow
	inc     si
	dec     ah
	jne     @@next
	ret
@@stepColumn:
	dec     bx
	add     dh, ch
	jae     @@store
@@stepRow:
	add     bx, di
@@store:
	lodsb
	mov     byte ptr es:[bx], al
	dec     ah
	jne     @@next
	ret
TurnedBackSpanLoop ENDP

TurnedBackClipSpanLoop PROC NEAR
	test    byte ptr ss:ClipEdges, CLIP_LEFT OR CLIP_BOTTOM
	je      TurnedBackSpanLoop
	mov     di, word ptr ss:RowPitch
	jmp     @@store
@@next:
	add     dl, cl
	jb      @@stepColumn
	add     dh, ch
	jb      @@stepRow
	inc     si
	dec     ah
	jne     @@next
	ret
@@stepColumn:
	dec     word ptr ss:ColumnsLeft
	jle     @@done
	dec     bx
	add     dh, ch
	jae     @@store
@@stepRow:
	dec     word ptr ss:RowsLeft
	jle     @@done
	add     bx, di
@@store:
	lodsb
	mov     byte ptr es:[bx], al
	dec     ah
	jne     @@next
@@done:
	ret
TurnedBackClipSpanLoop ENDP

TurnedBackSpanFill PROC NEAR
	mov     di, word ptr ss:RowPitch
	jmp     @@store
@@next:
	add     dl, cl
	jb      @@stepColumn
	add     dh, ch
	jb      @@stepRow
	lodsb
	dec     ah
	jne     @@next
	ret
@@stepColumn:
	dec     bx
	add     dh, ch
	jae     @@store
	mov     byte ptr es:[bx], al
@@stepRow:
	add     bx, di
@@store:
	lodsb
	mov     byte ptr es:[bx], al
	dec     ah
	jne     @@next
	ret
TurnedBackSpanFill ENDP

TurnedBackClipSpanFill PROC NEAR
	mov     di, word ptr ss:RowPitch
	test    byte ptr ss:ClipEdges, CLIP_RIGHT
	jne     @@clippedStart
	test    byte ptr ss:ClipEdges, CLIP_LEFT OR CLIP_BOTTOM
	je      TurnedBackSpanFill
	jmp     short @@store
@@clippedStart:
	cmp     bp, word ptr ss:ClipTop
	je      @@store
	push    si
	push    dx
@@back:
	dec     si
	sub     dl, cl
	jae     @@backRow
	sub     dh, ch
	jae     @@backDone
	sub     bx, di
	mov     al, byte ptr [si]
	mov     byte ptr es:[bx], al
	add     bx, di
	jmp     short @@backDone
@@backRow:
	sub     dh, ch
	jae     @@back
@@backDone:
	pop     dx
	pop     si
	jmp     @@store
@@next:
	add     dl, cl
	jb      @@stepColumn
	add     dh, ch
	jb      @@stepRow
	lodsb
	dec     ah
	jne     @@next
	ret
@@stepColumn:
	dec     word ptr ss:ColumnsLeft
	jle     @@done
	dec     bx
	add     dh, ch
	jae     @@store
	mov     byte ptr es:[bx], al
@@stepRow:
	add     bx, di
	dec     word ptr ss:RowsLeft
	jle     @@done
@@store:
	lodsb
	mov     byte ptr es:[bx], al
	dec     ah
	jne     @@next
@@done:
	ret
TurnedBackClipSpanFill ENDP

TurnedSpanMirrorLoop PROC NEAR
	mov     di, word ptr ss:RowPitch
	jmp     @@store
@@next:
	add     dl, cl
	jb      @@stepColumn
	add     dh, ch
	jb      @@stepRow
	inc     si
	dec     ah
	jne     @@next
	ret
@@stepColumn:
	dec     bx
	add     dh, ch
	jae     @@store
@@stepRow:
	sub     bx, di
@@store:
	lodsb
	mov     byte ptr es:[bx], al
	dec     ah
	jne     @@next
	ret
TurnedSpanMirrorLoop ENDP

TurnedClipSpanMirrorLoop PROC NEAR
	test    byte ptr ss:ClipEdges, CLIP_LEFT OR CLIP_TOP
	je      TurnedSpanMirrorLoop
	mov     di, word ptr ss:RowPitch
	jmp     @@store
@@next:
	add     dl, cl
	jb      @@stepColumn
	add     dh, ch
	jb      @@stepRow
	inc     si
	dec     ah
	jne     @@next
	ret
@@stepColumn:
	dec     word ptr ss:ColumnsLeft
	jle     @@done
	dec     bx
	add     dh, ch
	jae     @@store
@@stepRow:
	dec     word ptr ss:RowsLeft
	jle     @@done
	sub     bx, di
@@store:
	lodsb
	mov     byte ptr es:[bx], al
	dec     ah
	jne     @@next
@@done:
	ret
TurnedClipSpanMirrorLoop ENDP

TurnedSpanMirrorFill PROC NEAR
	mov     di, word ptr ss:RowPitch
	jmp     @@store
@@next:
	add     dl, cl
	jb      @@stepColumn
	add     dh, ch
	jb      @@stepRow
	lodsb
	dec     ah
	jne     @@next
	ret
@@stepColumn:
	add     dh, ch
	jae     @@moveColumn
	sub     bx, di
	mov     byte ptr es:[bx], al
@@moveColumn:
	dec     bx
	lodsb
	mov     byte ptr es:[bx], al
	dec     ah
	jne     @@next
	ret
@@stepRow:
	sub     bx, di
@@store:
	lodsb
	mov     byte ptr es:[bx], al
	dec     ah
	jne     @@next
	ret
TurnedSpanMirrorFill ENDP

TurnedClipSpanMirrorFill PROC NEAR
	mov     di, word ptr ss:RowPitch
	test    byte ptr ss:ClipEdges, CLIP_BOTTOM
	jne     @@clippedStart
	test    byte ptr ss:ClipEdges, CLIP_LEFT OR CLIP_TOP
	je      TurnedSpanMirrorFill
	jmp     short @@store
@@clippedStart:
	cmp     bp, word ptr ss:ClipRight
	jge     @@store
	push    si
	push    dx
@@back:
	dec     si
	sub     dl, cl
	jae     @@backRow
	sub     dh, ch
	jae     @@backDone
	inc     bx
	mov     al, byte ptr [si]
	mov     byte ptr es:[bx], al
	dec     bx
	jmp     short @@backDone
@@backRow:
	sub     dh, ch
	jae     @@back
@@backDone:
	pop     dx
	pop     si
	jmp     @@store
@@next:
	add     dl, cl
	jb      @@stepColumn
	add     dh, ch
	jb      @@stepRow
	lodsb
	dec     ah
	jne     @@next
	ret
@@stepColumn:
	add     dh, ch
	jae     @@moveColumn
	dec     word ptr ss:RowsLeft
	jle     @@done
	sub     bx, di
	mov     byte ptr es:[bx], al
@@moveColumn:
	dec     word ptr ss:ColumnsLeft
	jle     @@done
	dec     bx
	lodsb
	mov     byte ptr es:[bx], al
	dec     ah
	jne     @@next
	ret
@@stepRow:
	sub     bx, di
	dec     word ptr ss:RowsLeft
	jle     @@done
@@store:
	lodsb
	mov     byte ptr es:[bx], al
	dec     ah
	jne     @@next
@@done:
	ret
TurnedClipSpanMirrorFill ENDP

; Turned 91 to 179 degrees, as ShrinkTurned.
ShrinkTurnedBack PROC FAR
	call    far ptr PlaceTurnedBack
	jb      @@offView
	call    far ptr FrameNeedsClip
	jae     @@inside
	mov     word ptr ss:SpanProc, offset TurnedBackClipSpan
	jmp     TurnedBackRows
@@offView:
	DRAW_EXIT
@@inside:
	mov     word ptr ss:SpanProc, offset TurnedBackSpan
	jmp     TurnedBackRows
ShrinkTurnedBack ENDP

ShrinkTurnedBackMirror PROC FAR
	call    far ptr PlaceTurnedBackMirror
	jb      @@offView
	call    far ptr FrameNeedsClip
	jae     @@inside
	mov     word ptr ss:SpanProc, offset TurnedBackClipSpanMirror
	jmp     TurnedBackRows
@@offView:
	DRAW_EXIT
@@inside:
	mov     word ptr ss:SpanProc, offset TurnedBackSpanMirror
ShrinkTurnedBackMirror ENDP

TurnedBackRows PROC FAR
	xor     dx, dx
	mov     word ptr ss:StepFraction, dx
@@rowStart:
	mov     word ptr ss:ScreenX, bx
	mov     word ptr ss:ScreenY, bp
@@nextSpan:
	mov     ax, [si].span_length
	or      ax, ax
	jne     @@haveSpan
	jmp     @@done
@@haveSpan:
	mov     dx, [si].span_y
	cmp     dx, word ptr ss:SpanRow
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
	jmp     @@nextSpan
@@rowDone:
	mov     bx, word ptr ss:ScreenX
	mov     bp, word ptr ss:ScreenY
	mov     dx, word ptr ss:StepFraction
@@nextRow:
	inc     word ptr ss:SpanRow
	dec     word ptr ss:FrameHeight
	je      @@done
	mov     ax, PLAIN_LOOP
	add     dl, ch
	jae     @@sameColumn
	dec     bx
	jmp     short @@checkRow
@@sameColumn:
	add     dh, cl
	jb      @@stepRow
	jae     @@nextRow
@@checkRow:
	add     dh, cl
	jae     @@stepped
@@stepRow:
	dec     bp
@@stepped:
	mov     word ptr ss:StepFraction, dx
	cmp     cl, ch
	jge     @@rowFirst
@@columnFirst:
	add     dl, ch
	jb      @@gaps
	add     dh, cl
	jb      @@setFill
	jmp     @@columnFirst
@@rowFirst:
	add     dh, cl
	jb      @@gaps
	add     dl, ch
	jb      @@setFill
	jmp     @@rowFirst
@@gaps:
	mov     ax, FILL_LOOP
@@setFill:
	mov     word ptr ss:FillGaps, ax
	jmp     @@rowStart
@@done:
	DRAW_EXIT
TurnedBackRows ENDP

ShrinkTurnedBackFlip PROC FAR
	call    far ptr PlaceTurnedBackFlip
	jb      @@offView
	call    far ptr FrameNeedsClip
	jae     @@inside
	mov     word ptr ss:SpanProc, offset TurnedBackClipSpan
	jmp     TurnedBackRowsFlip
@@offView:
	DRAW_EXIT
@@inside:
	mov     word ptr ss:SpanProc, offset TurnedBackSpan
	jmp     TurnedBackRowsFlip
ShrinkTurnedBackFlip ENDP

ShrinkTurnedBackBoth PROC FAR
	call    far ptr PlaceTurnedBackBoth
	jb      @@offView
	call    far ptr FrameNeedsClip
	jae     @@inside
	mov     word ptr ss:SpanProc, offset TurnedBackClipSpanMirror
	jmp     TurnedBackRowsFlip
@@offView:
	DRAW_EXIT
@@inside:
	mov     word ptr ss:SpanProc, offset TurnedBackSpanMirror
ShrinkTurnedBackBoth ENDP

TurnedBackRowsFlip PROC FAR
	xor     dx, dx
@@rowStart:
	mov     word ptr ss:StepFraction, dx
	mov     word ptr ss:ScreenX, bx
	mov     word ptr ss:ScreenY, bp
@@nextSpan:
	mov     ax, [si].span_length
	or      ax, ax
	jne     @@haveSpan
	jmp     @@done
@@haveSpan:
	mov     dx, [si].span_y
	cmp     dx, word ptr ss:SpanRow
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
	jmp     @@nextSpan
@@rowDone:
	mov     bx, word ptr ss:ScreenX
	mov     bp, word ptr ss:ScreenY
	mov     dx, word ptr ss:StepFraction
@@nextRow:
	inc     word ptr ss:SpanRow
	dec     word ptr ss:FrameHeight
	je      @@done
	mov     ax, PLAIN_LOOP
	add     dl, ch
	jae     @@sameColumn
	inc     bx
	cmp     cl, ch
	jl      @@checkRow
	mov     ax, FILL_LOOP
@@checkRow:
	add     dh, cl
	jb      @@stepRow
	jmp     short @@setFill
@@sameColumn:
	add     dh, cl
	jae     @@nextRow
@@stepRow:
	inc     bp
	cmp     cl, ch
	jge     @@setFill
	mov     ax, FILL_LOOP
@@setFill:
	mov     word ptr ss:FillGaps, ax
	jmp     @@rowStart
@@done:
	DRAW_EXIT
TurnedBackRowsFlip ENDP

	END
