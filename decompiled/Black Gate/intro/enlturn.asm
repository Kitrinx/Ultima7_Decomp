; Black Gate INTRO.EXE, resident segment 74 (file offsets 0x014ab6 to 0x017654, 11166 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

; Draws a shape frame enlarged (scale 100h or more) and turned by 1 to 89 or
; 91 to 179 degrees, in each of the four flips. Every source pixel becomes a
; patch of screen pixels, drawn as strips along rows or columns.

	.MODEL  MEDIUM
	LOCALS

	INCLUDE scalfram.inc

	.CODE

; Turned 1 to 89 degrees, mirrored: spans run left and up, rows step left
; and down.
; Each entry places the frame, then picks the span routines: clipped ones when
; the frame needs clipping, drawn by rows when the turn is steeper than 45
; degrees. The walker draws the rows.
EnlargeTurnedMirror PROC FAR
	call    far ptr PlaceEnlargedTurnedMirror  ; carry: off the view
	jb      @@hidden
	call    far ptr FrameNeedsClip  ; carry: needs clipping
	jae     @@inside
	mov     ss:SpanProc, offset ClipStartUpLeft
	mov     ax, offset ClipRowsUpLeft
	cmp     bl, bh                  ; steeper than 45 degrees?
	jb      @@clipStep
	mov     ax, offset ClipColumnsUpLeft
@@clipStep:
	mov     ss:StepProc, ax
	jmp     WalkLeftDown
@@hidden:
	DRAW_EXIT
@@inside:
	mov     ss:SpanProc, offset StartUpLeft
	mov     ax, offset RowsUpLeft
	cmp     bl, bh
	jb      @@step
	mov     ax, offset ColumnsUpLeft
@@step:
	mov     ss:StepProc, ax
	jmp     WalkLeftDown
EnlargeTurnedMirror ENDP

; Turned 1 to 89 degrees: spans run right and down, rows step left and down.
EnlargeTurned PROC FAR
	call    far ptr PlaceEnlargedTurned  ; carry: off the view
	jb      @@hidden
	call    far ptr FrameNeedsClip  ; carry: needs clipping
	jae     @@inside
	mov     ss:SpanProc, offset ClipStartDownRight
	mov     ax, offset ClipRowsDownRight
	cmp     bl, bh                  ; steeper than 45 degrees?
	jb      @@clipStep
	mov     ax, offset ClipColumnsDownRight
@@clipStep:
	mov     ss:StepProc, ax
	jmp     WalkLeftDown
@@hidden:
	DRAW_EXIT
@@inside:
	mov     ss:SpanProc, offset StartDownRight
	mov     ax, offset RowsDownRight
	cmp     bl, bh
	jb      @@step
	mov     ax, offset ColumnsDownRight
@@step:
	mov     ss:StepProc, ax
EnlargeTurned ENDP

; Walk the frame's rows from the top. The pen moves left and down by
; one scaled row before drawing each row; each span on the current source row is
; unpacked into SpanBuffer when run-length encoded and passed to SpanStart
; with AX at its x offset, BX the turn fractions and DS:SI at its pixels.
WalkLeftDown PROC FAR
	mov     ss:StepFraction, 0
; Add one scaled row to the fractions: CL and CH take the whole steps.
@@row:
	mov     dx, ss:SinStep
	mov     cx, ss:CosStep
	add     byte ptr ss:StepFraction, dl  ; whole pixels the pen moves
	adc     dh, 0
	add     byte ptr ss:StepFraction+1, cl
	adc     ch, 0
	mov     cl, dh
	mov     word ptr ss:CosWhole, cx
	or      cx, cx
	jne     @@move
	jmp     @@rowDone
@@move:
	xor     ax, ax
	mov     al, cl
	sub     ss:ScreenX, ax
	mov     al, ch
	add     ss:ScreenY, ax
@@span:
	mov     ax, [si].span_length    ; 0 ends the frame
	or      ax, ax
	jne     @@haveSpan
	jmp     @@done
@@haveSpan:
	mov     dx, [si].span_y
	cmp     dx, ss:SpanRow
	je      @@thisRow
	jl      @@skipSpan
	jmp     @@rowDone
@@skipSpan:
	add     si, SIZE SPANHDR
	shr     ax, 1                   ; odd: run-length encoded
	jb      @@skipRuns
	add     si, ax
	jmp     short @@skipped
@@skipRuns:
	push    di
	mov     di, ax
@@skipRun:
	lodsb
	inc     si
	shr     al, 1                   ; run byte: count*2 + 1 if a fill
	cbw
	jb      @@skippedRun
	add     si, ax
	dec     si
@@skippedRun:
	sub     di, ax
	jne     @@skipRun
	pop     di
@@skipped:
	jmp     @@span
@@thisRow:
	shr     ax, 1                   ; pixel count; carry: run-length encoded
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
; Unpack the runs into SpanBuffer and draw from there.
@@unpack:
	lodsb
	shr     al, 1                   ; run byte: count*2 + 1 if a fill
	cbw
	mov     cx, ax
	jb      @@unpackFill
	shr     cx, 1
	rep     movsw
	rcl     cx, 1                   ; and the odd byte
	rep     movsb
	sub     bx, ax
	jne     @@unpack
	je      @@unpacked
@@unpackFill:
	sub     bx, ax
	lodsb
	mov     ah, al
	shr     cx, 1
	rep     stosw
	rcl     cx, 1                   ; and the odd byte
	rep     stosb
	or      bx, bx
	jne     @@unpack
@@unpacked:
	pop     es
	pop     ax
	mov     bx, word ptr ss:ScaledCos  ; BL cos, BH sin
	push    si
	push    ds
	push    ss
	pop     ds
	mov     si, offset SpanBuffer
	jmp     short @@draw
@@raw:
	mov     di, ax
	mov     ax, [si].span_x
	add     si, SIZE SPANHDR
	add     di, si
	push    di
	push    ds
@@draw:
	call    ss:SpanProc
	pop     ds
	pop     si
	jmp     @@span
; Next source row.
@@rowDone:
	inc     ss:SpanRow
	dec     ss:FrameHeight
	je      @@done
	jmp     @@row
@@done:
	DRAW_EXIT
WalkLeftDown ENDP

; Turned 1 to 89 degrees, flipped: spans run right and down, rows step right
; and up.
EnlargeTurnedFlip PROC FAR
	call    far ptr PlaceEnlargedTurnedFlip  ; carry: off the view
	jb      @@hidden
	call    far ptr FrameNeedsClip  ; carry: needs clipping
	jae     @@inside
	mov     ss:SpanProc, offset ClipStartDownRight
	mov     ax, offset ClipRowsDownRight
	cmp     bl, bh                  ; steeper than 45 degrees?
	jb      @@clipStep
	mov     ax, offset ClipColumnsDownRight
@@clipStep:
	mov     ss:StepProc, ax
	jmp     WalkRightUp
@@hidden:
	DRAW_EXIT
@@inside:
	mov     ss:SpanProc, offset StartDownRight
	mov     ax, offset RowsDownRight
	cmp     bl, bh
	jb      @@step
	mov     ax, offset ColumnsDownRight
@@step:
	mov     ss:StepProc, ax
	jmp     WalkRightUp
EnlargeTurnedFlip ENDP

; Turned 1 to 89 degrees, mirrored and flipped: spans run left and up, rows
; step right and up.
EnlargeTurnedBoth PROC FAR
	call    far ptr PlaceEnlargedTurnedBoth  ; carry: off the view
	jb      @@hidden
	call    far ptr FrameNeedsClip  ; carry: needs clipping
	jae     @@inside
	mov     ss:SpanProc, offset ClipStartUpLeft
	mov     ax, offset ClipRowsUpLeft
	cmp     bl, bh                  ; steeper than 45 degrees?
	jb      @@clipStep
	mov     ax, offset ClipColumnsUpLeft
@@clipStep:
	mov     ss:StepProc, ax
	jmp     WalkRightUp
@@hidden:
	DRAW_EXIT
@@inside:
	mov     ss:SpanProc, offset StartUpLeft
	mov     ax, offset RowsUpLeft
	cmp     bl, bh
	jb      @@step
	mov     ax, offset ColumnsUpLeft
@@step:
	mov     ss:StepProc, ax
EnlargeTurnedBoth ENDP

; As WalkLeftDown, moving the pen right and up after each row.
WalkRightUp PROC FAR
	xor     dx, dx
	mov     ss:StepFraction, dx
; Add one scaled row to the fractions: CL and CH take the whole steps.
@@row:
	mov     dx, ss:SinStep
	mov     cx, ss:CosStep
	add     byte ptr ss:StepFraction, dl  ; whole pixels the pen moves
	adc     dh, 0
	add     byte ptr ss:StepFraction+1, cl
	adc     ch, 0
	mov     cl, dh
	mov     word ptr ss:CosWhole, cx
	or      cx, cx
	jne     @@span
	jmp     @@rowDone
@@span:
	mov     ax, [si].span_length    ; 0 ends the frame
	or      ax, ax
	jne     @@haveSpan
	jmp     @@done
@@haveSpan:
	mov     dx, [si].span_y
	cmp     dx, ss:SpanRow
	je      @@thisRow
	jl      @@skipSpan
	jmp     @@rowDone
@@skipSpan:
	add     si, SIZE SPANHDR
	shr     ax, 1                   ; odd: run-length encoded
	jb      @@skipRuns
	add     si, ax
	jmp     short @@skipped
@@skipRuns:
	push    di
	mov     di, ax
@@skipRun:
	lodsb
	inc     si
	shr     al, 1                   ; run byte: count*2 + 1 if a fill
	cbw
	jb      @@skippedRun
	add     si, ax
	dec     si
@@skippedRun:
	sub     di, ax
	jne     @@skipRun
	pop     di
@@skipped:
	jmp     @@span
@@thisRow:
	shr     ax, 1                   ; pixel count; carry: run-length encoded
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
; Unpack the runs into SpanBuffer and draw from there.
@@unpack:
	lodsb
	shr     al, 1                   ; run byte: count*2 + 1 if a fill
	cbw
	mov     cx, ax
	jb      @@unpackFill
	shr     cx, 1
	rep     movsw
	rcl     cx, 1                   ; and the odd byte
	rep     movsb
	sub     bx, ax
	jne     @@unpack
	je      @@unpacked
@@unpackFill:
	sub     bx, ax
	lodsb
	mov     ah, al
	shr     cx, 1
	rep     stosw
	rcl     cx, 1                   ; and the odd byte
	rep     stosb
	or      bx, bx
	jne     @@unpack
@@unpacked:
	pop     es
	pop     ax
	mov     bx, word ptr ss:ScaledCos  ; BL cos, BH sin
	push    si
	push    ds
	push    ss
	pop     ds
	mov     si, offset SpanBuffer
	jmp     short @@draw
@@raw:
	mov     di, ax
	mov     ax, [si].span_x
	add     si, SIZE SPANHDR
	add     di, si
	push    di
	push    ds
@@draw:
	call    ss:SpanProc
	pop     ds
	pop     si
	jmp     @@span
; Next source row.
@@rowDone:
	inc     ss:SpanRow
	dec     ss:FrameHeight
	je      @@done
	mov     cx, word ptr ss:CosWhole
	xor     ax, ax
	mov     al, cl
	add     ss:ScreenX, ax
	mov     al, ch
	sub     ss:ScreenY, ax
	jmp     @@row
@@done:
	DRAW_EXIT
WalkRightUp ENDP

; Find the screen address of a span's first pixel: its x offset from the
; frame's left edge, scaled, turned and added to the pen, going right and down.
; Leaves the address in DI and the fractions in BX and DL for SpanRunner.
StartDownRight PROC NEAR
	add     ax, ss:FrameLeft        ; x offset from the left edge
	mul     ss:CurrentScale
	mov     al, ah                  ; scaled: whole pixels
	mov     ah, dl
	push    ax
	xor     cx, cx
	mov     cl, bh
	mul     cx                      ; times sin
	mov     ss:YFraction, al
	mov     ss:SinFraction, al
	mov     al, ah
	mov     ah, dl
	add     ax, ss:ScreenY
	shl     ax, 1                   ; row offset from the view's table
	add     ax, ss:ViewRows
	mov     di, ax
	mov     di, ss:[di]
	pop     ax
	mov     cl, bl
	mul     cx                      ; times cos
	mov     ss:XFraction, al
	mov     ss:CosFraction, al
	mov     cl, al
	mov     al, ah
	mov     ah, dl
	add     ax, ss:ScreenX
	add     di, ax
	mov     dl, cl
	jmp     ss:StepProc
StartDownRight ENDP

; Draw a span that runs more down than across, right and down: each source pixel
; becomes a strip on every screen row it covers, the strips sliding sideways
; by the turn and widening by the scale.
RowsDownRight PROC NEAR
	mov     cx, ss:SinStep
	mov     al, ss:YFraction
	add     ss:SinFraction, cl      ; rows this pixel covers
	adc     ch, 0
	mov     ss:SinCount, ch
	mov     cx, word ptr ss:CosWhole
	mov     dh, dl
	or      ch, ch
	je      @@measured
; Width of a strip: the whole steps of a row, spread by the turn.
@@measure:
	add     dh, bl
	adc     cl, 0
	add     ah, bh
	sbb     ch, 0
	jne     @@measure
@@measured:
	mov     ch, al
RowsDownRightInside:
	cmp     ss:SinCount, 0
	jle     @@pixelDone
@@pixel:
	mov     al, [si]
@@strip:
	mov     bp, cx
	xor     ch, ch
	rep     stosb
	mov     cx, bp
	add     di, ss:RowPitch         ; next row, back to the strip's start
	xor     ch, ch
	sub     di, cx
	mov     cx, bp
; Slide the strip by the turn, and widen it by the scale.
@@slide:
	add     dl, bl
	jae     @@slideCarry
	dec     cl
	inc     di
@@slideCarry:
	add     ch, bh
	jae     @@slide
@@widen:
	add     dh, bl
	adc     cl, 0
	add     ah, bh
	jae     @@widen
	dec     ss:SinCount
	jne     @@strip
@@pixelDone:
	mov     bp, cx
	mov     cx, ss:SinStep
@@nextPixel:
	inc     si                      ; next source pixel
	add     ss:SinFraction, cl
	adc     ch, 0
	dec     ss:RunLength
	jle     @@done
	or      ch, ch
	je      @@nextPixel
	mov     ss:SinCount, ch
	mov     cx, bp
	jmp     @@pixel
@@done:
	ret
RowsDownRight ENDP

; Draw a span that runs more across than down, right and down: each source pixel
; becomes a strip on every screen column it covers.
ColumnsDownRight PROC NEAR
	mov     cx, ss:CosStep
	add     ss:CosFraction, cl      ; columns this pixel covers
	adc     ch, 0
	mov     ss:CosCount, ch
	mov     cx, word ptr ss:CosWhole
	mov     ah, ss:YFraction
	xchg    ch, cl
	mov     dh, dl
	or      ch, ch
	je      @@measured
; Height of a strip: the whole steps of a row, spread by the turn.
@@measure:
	sub     ah, bh
	adc     cl, 0
	sub     dh, bl
	sbb     ch, 0
	jne     @@measure
@@measured:
	mov     ch, ss:YFraction
ColumnsDownRightInside:
	mov     bp, ss:RowPitch
	mov     al, [si]
	cmp     ss:CosCount, 0
	je      @@pixelDone
@@pixel:
	mov     al, [si]
@@column:
	push    cx
	push    di
@@plot:
	mov     byte ptr es:[di], al
	sub     di, bp
	dec     cl
	jg      @@plot
	pop     di
	pop     cx
	inc     di
@@slide:
	add     ch, bh
	jae     @@slideCarry
	inc     cl
	add     di, bp
@@slideCarry:
	add     dl, bl
	jae     @@slide
@@widen:
	add     ah, bh
	sbb     cl, 0
	add     dh, bl
	jae     @@widen
	dec     ss:CosCount
	jne     @@column
@@pixelDone:
	mov     bp, cx
	mov     cx, ss:CosStep
@@nextPixel:
	inc     si                      ; next source pixel
	add     ss:CosFraction, cl
	adc     ch, 0
	dec     ss:RunLength
	jle     @@done
	or      ch, ch
	jle     @@nextPixel
	mov     ss:CosCount, ch
	mov     cx, bp
	mov     bp, ss:RowPitch
	jmp     @@pixel
@@done:
	ret
ColumnsDownRight ENDP

; As StartDownRight, going left and up.
StartUpLeft PROC NEAR
	add     ax, ss:FrameLeft
	mul     ss:CurrentScale
	mov     al, ah
	mov     ah, dl
	push    ax
	xor     cx, cx
	mov     cl, bh
	mul     cx
	mov     ss:YFraction, al
	mov     ss:SinFraction, al
	mov     al, ah
	mov     ah, dl
	neg     ax
	add     ax, ss:ScreenY
	shl     ax, 1
	add     ax, ss:ViewRows
	mov     di, ax
	mov     di, ss:[di]
	pop     ax
	mov     cl, bl
	mul     cx
	mov     ss:XFraction, al
	mov     ss:CosFraction, al
	mov     cl, al
	mov     al, ah
	mov     ah, dl
	neg     ax
	add     ax, ss:ScreenX
	add     di, ax
	mov     dl, cl
	jmp     ss:StepProc
StartUpLeft ENDP

; As RowsDownRight, going left and up.
RowsUpLeft PROC NEAR
	mov     cx, ss:SinStep
	mov     al, ss:YFraction
	add     ss:SinFraction, cl      ; rows this pixel covers
	adc     ch, 0
	mov     ss:SinCount, ch
	mov     ah, ss:YFraction
	mov     ch, ah
	mov     dl, ss:XFraction
	mov     dh, dl
	mov     cl, ss:CosWhole
	mov     al, ss:SinWhole
	or      al, al
	je      @@measured
@@measure:
	sub     dh, bl
	adc     cl, 0
	sub     ah, bh
	sbb     al, 0
	jg      @@measure
@@measured:
	mov     ch, al
RowsUpLeftInside:
	cmp     ss:SinCount, 0
	jle     @@pixelDone
@@pixel:
	mov     al, [si]
@@strip:
	mov     bp, cx
	xor     ch, ch
	rep     stosb
	mov     cx, bp
	sub     di, ss:RowPitch
	xor     ch, ch
	sub     di, cx
	mov     cx, bp
; Slide the strip by the turn, and widen it by the scale.
@@slide:
	add     dl, bl
	jae     @@slideCarry
	inc     cl
	dec     di
@@slideCarry:
	add     ch, bh
	jae     @@slide
@@widen:
	add     dh, bl
	sbb     cl, 0
	add     ah, bh
	jae     @@widen
	dec     ss:SinCount
	jne     @@strip
@@pixelDone:
	mov     bp, cx
	mov     cx, ss:SinStep
@@nextPixel:
	inc     si                      ; next source pixel
	add     ss:SinFraction, cl
	adc     ch, 0
	dec     ss:RunLength
	jle     @@done
	or      ch, ch
	je      @@nextPixel
	mov     ss:SinCount, ch
	mov     cx, bp
	jmp     @@pixel
@@done:
	ret
RowsUpLeft ENDP

; As ColumnsDownRight, going left and up.
ColumnsUpLeft PROC NEAR
	mov     cx, ss:CosStep
	add     ss:CosFraction, cl      ; columns this pixel covers
	adc     ch, 0
	mov     ss:CosCount, ch
	mov     cx, word ptr ss:CosWhole
	mov     ah, ss:YFraction
	xchg    ch, cl
	mov     dh, dl
	or      ch, ch
	je      @@measured
@@measure:
	add     ah, bh
	adc     cl, 0
	add     dh, bl
	sbb     ch, 0
	jg      @@measure
@@measured:
	mov     ch, ss:YFraction
ColumnsUpLeftInside:
	mov     bp, ss:RowPitch
	mov     al, [si]
	cmp     ss:CosCount, 0
	je      @@pixelDone
@@pixel:
	mov     al, [si]
@@column:
	push    cx
	push    di
@@plot:
	mov     byte ptr es:[di], al
	sub     di, bp
	dec     cl
	jg      @@plot
	pop     di
	pop     cx
	dec     di
@@slide:
	add     ch, bh
	jae     @@slideCarry
	dec     cl
	sub     di, bp
@@slideCarry:
	add     dl, bl
	jae     @@slide
@@widen:
	add     ah, bh
	adc     cl, 0
	add     dh, bl
	jae     @@widen
	dec     ss:CosCount
	jne     @@column
@@pixelDone:
	mov     bp, cx
	mov     cx, ss:CosStep
@@nextPixel:
	inc     si                      ; next source pixel
	add     ss:CosFraction, cl
	adc     ch, 0
	dec     ss:RunLength
	jle     @@done
	or      ch, ch
	jle     @@nextPixel
	mov     ss:CosCount, ch
	mov     cx, bp
	mov     bp, ss:RowPitch
	jmp     @@pixel
@@done:
	ret
ColumnsUpLeft ENDP

; As StartDownRight, for a frame the clip box cuts: keeps the first pixel's
; position in DrawX and DrawY, and skips ahead to the top edge when the span
; starts beyond it.
ClipStartDownRight PROC NEAR
	add     ax, ss:FrameLeft
	mul     ss:CurrentScale
	mov     al, ah
	mov     ah, dl
	push    ax
	xor     cx, cx
	mov     cl, bh
	mul     cx
	mov     ss:YFraction, al
	mov     cx, ss:SinStep
	add     al, cl
	adc     ch, 0
	mov     ss:SinCount, ch
	mov     ss:SinFraction, al
	mov     al, ah
	mov     ah, dl
	add     ax, ss:ScreenY
	mov     ss:LineY, ax
	mov     di, ax
	pop     ax
	xor     cx, cx
	mov     cl, bl
	mul     cx
	mov     ss:XFraction, al
	mov     ss:CosFraction, al
	mov     cl, al
	mov     al, ah
	mov     ah, dl
	add     ax, ss:ScreenX
	mov     ss:LineX, ax
	mov     bp, ax
	mov     ss:DriftX, 0            ; nothing cut yet
	mov     ss:DriftY, 0
	mov     ss:ClipEdges, 0
	cmp     ax, ss:ClipRight        ; starts past the far side?
	jle     @@inReach
	jmp     @@done
@@inReach:
	mov     ax, ss:LineY
	sub     ax, ss:ClipTop          ; rows above the clip box
	jge     @@run
	or      ax, ax
	jns     @@distance
	neg     ax
@@distance:
	xor     dx, dx
	mov     dl, ah
	mov     ah, al
	xor     al, al
	sub     al, ss:YFraction
	sbb     ah, 0
	sbb     dx, 0
	xor     cx, cx
	mov     cl, bh
	div     cx                      ; source steps to reach the edge
	or      dx, dx
	je      @@advance
	inc     ax
@@advance:
	push    ax
	mul     cx
	add     ss:YFraction, al
	adc     ah, 0
	adc     dx, 0
	mov     al, ah
	mov     ah, dl
	add     ss:DriftY, ax
	add     ss:LineY, ax
	pop     ax
	xor     cx, cx
	mov     cl, bl
	mul     cx
	add     ss:XFraction, al
	adc     ah, 0
	adc     dx, 0
	mov     al, ah
	mov     ah, dl
	add     ss:LineX, ax
	add     ss:DriftX, ax
	or      ss:ClipEdges, CLIP_TOP
@@run:
	jmp     ss:StepProc
@@done:
	ret
ClipStartDownRight ENDP

; As RowsDownRight, clipped to the view. Pixels wholly before the clip box are
; skipped; the lead loop cuts strips at the near edge, the body draws whole
; strips while watching the room left, and the tail cuts them at the far
; edge. The body hands over to RowsDownRightInside once no edge is left.
ClipRowsDownRight PROC NEAR
; Width of one scaled pixel across.
	xor     ax, ax
	mov     al, ss:CosWhole
	mov     ch, ss:SinWhole
	xor     dx, dx
	or      ch, ch
	je      @@measured
@@measure:
	add     dl, bl
	adc     ax, 0
	add     dh, bh
	sbb     ch, 0
	jg      @@measure
@@measured:
	inc     ax
	mov     ss:SpanWidth, ax
	mov     dx, ax
	mov     ax, ss:LineX
	add     ax, dx
	sub     ax, ss:ClipLeft         ; ends before the near side?
	jl      @@startOut
	jmp     @@startIn
@@startOut:
	or      ax, ax
	jns     @@startDistance
	neg     ax
@@startDistance:
	xor     dx, dx
	mov     dl, ah
	mov     ah, al
	xor     al, al
	sub     al, ss:XFraction
	sbb     ah, 0
	sbb     dx, 0
	xor     cx, cx
	mov     cl, bl
	div     cx
	or      dx, dx
	je      @@startSkip
	inc     ax
@@startSkip:
	push    ax
	mul     cx
	add     ss:XFraction, al
	adc     ah, 0
	adc     dx, 0
	mov     al, ah
	mov     ah, dl
	add     ss:LineX, ax
	pop     ax
	mov     cl, bh
	mul     cx
	add     ss:YFraction, al
	adc     ah, 0
	adc     dx, 0
	mov     al, ah
	mov     ah, dl
	add     ss:LineY, ax
	add     ss:DriftY, ax
	or      ss:ClipEdges, CLIP_LEFT
@@startIn:
	mov     ax, ss:ClipRight
	inc     ax
	sub     ax, ss:LineX            ; room to the far side
	jg      @@sideRoom
@@missed:
	ret
@@sideRoom:
	mov     ss:ColumnsLeft, ax
	mov     ax, ss:RunLength
	mul     ss:CosStep
	add     al, ss:CosFraction
	adc     ah, 0
	adc     dx, 0
	mov     al, ah
	mov     ah, dl
	add     ax, bp
	add     ax, ss:SpanWidth
	cmp     ax, ss:ClipLeft
	jl      @@missed
	cmp     ax, ss:ClipRight
	jle     @@sideDone
	or      ss:ClipEdges, CLIP_RIGHT
@@sideDone:
	mov     ax, ss:ClipBottom
	inc     ax
	sub     ax, ss:LineY            ; room to the bottom
	mov     ss:RowsLeft, ax
	jle     @@gone
	mov     ax, ss:RunLength
	mul     ss:SinStep
	add     al, ss:SinFraction
	adc     ah, 0
	adc     dx, 0
	mov     al, ah
	mov     ah, dl
	add     ax, di
	cmp     ax, ss:ClipTop
	jl      @@gone
	cmp     ax, ss:ClipBottom
	jle     @@edgesKnown
	or      ss:ClipEdges, CLIP_BOTTOM
; Skip the source pixels wholly before the clip box.
@@edgesKnown:
	test    ss:ClipEdges, CLIP_LEFT or CLIP_TOP
	je      @@noSkip
	mov     ax, ss:DriftY
	or      ax, ax
	je      @@noSkip
	xor     dx, dx
	mov     dl, ah
	mov     ah, al
	xor     al, al
	mov     al, ss:YFraction
	sub     al, ss:SinFraction
	sbb     ah, 0
	sbb     dx, 0
	mov     cx, ss:SinStep
	div     cx                      ; whole source pixels to skip
	add     si, ax
	sub     ss:RunLength, ax
	jg      @@skipPixels
@@gone:
	ret
@@skipPixels:
	mul     cx
	add     ss:SinFraction, al
	adc     ah, 0
	adc     dx, 0
	add     ss:SinFraction, cl
	adc     ch, 0
	mov     al, ah
	mov     ah, dl
	add     al, ch
	adc     ah, 0
	sub     ax, ss:DriftY
	mov     ss:SinCount, al
	jmp     short @@address
@@noSkip:
	mov     cx, ss:SinStep
	add     ss:SinFraction, cl
	adc     ch, 0
	mov     ss:SinCount, ch
; Screen address of the first pixel, and the strip width.
@@address:
	mov     ax, ss:LineY
	shl     ax, 1
	add     ax, ss:ViewRows
	mov     di, ax
	mov     di, ss:[di]
	add     di, ss:LineX
	mov     ah, ss:YFraction
	mov     ch, ah
	mov     dl, ss:XFraction
	mov     dh, dl
	mov     cl, ss:CosWhole
	mov     al, ss:SinWhole
	or      al, al
	je      @@widthDone
@@width:
	add     dh, bl
	adc     cl, 0
	add     ah, bh
	sbb     al, 0
	jg      @@width
@@widthDone:
	push    cx
	xor     ch, ch
	sub     ss:ColumnsLeft, cx
	pop     cx
	jg      @@leadFirst
	jmp     @@tailFirst
; Lead: strips cut at the near side.
@@leadFirst:
	cmp     ss:SinCount, 0
	jle     @@leadPixelDone
@@leadPixel:
	mov     al, [si]
@@leadStrip:
	push    di
	push    dx
	push    cx
	xor     ch, ch
	mov     dx, ss:ClipLeft
	sub     dx, ss:LineX
	jle     @@leadDone
	sub     cx, dx
	jle     @@leadStripDone
	add     di, dx
	rep     stosb
@@leadStripDone:
	pop     cx
	pop     dx
	pop     di
	add     di, ss:RowPitch
@@leadSlide:
	add     dl, bl
	jae     @@leadSlideCarry
	dec     cl
	inc     di
	inc     ss:LineX
@@leadSlideCarry:
	add     ch, bh
	jae     @@leadSlide
@@leadWiden:
	add     dh, bl
	jae     @@leadWidenCarry
	dec     ss:ColumnsLeft
	inc     cl
@@leadWidenCarry:
	add     ah, bh
	jae     @@leadWiden
	dec     ss:RowsLeft
	jle     @@leadOut
	cmp     ss:ColumnsLeft, 0
	jle     @@toTail
	dec     ss:SinCount
	jg      @@leadStrip
@@leadPixelDone:
	mov     bp, cx
	mov     cx, ss:SinStep
@@leadNextPixel:
	inc     si
	add     ss:SinFraction, cl
	adc     ch, 0
	dec     ss:RunLength
	jle     @@leadOut
	or      ch, ch
	je      @@leadNextPixel
	mov     ss:SinCount, ch
	mov     cx, bp
	jmp     @@leadPixel
@@leadOut:
	ret
@@toTail:
	jmp     @@tailRepeat
; Body: whole strips, watching the room left.
@@leadDone:
	pop     cx
	pop     dx
	pop     di
	test    ss:ClipEdges, CLIP_RIGHT or CLIP_BOTTOM
	jne     @@bodyFirst
	jmp     RowsDownRightInside     ; no edge left: the plain loop finishes
@@bodyFirst:
	cmp     ss:SinCount, 0
	jle     @@leadPixelDone
@@bodyPixel:
	mov     al, [si]
@@bodyStrip:
	mov     bp, cx
	xor     ch, ch
	rep     stosb
	mov     cx, bp
	add     di, ss:RowPitch
	xor     ch, ch
	sub     di, cx
	mov     cx, bp
@@bodySlide:
	add     dl, bl
	jae     @@bodySlideCarry
	dec     cl
	inc     di
	inc     ss:LineX
@@bodySlideCarry:
	add     ch, bh
	jae     @@bodySlide
@@bodyWiden:
	add     dh, bl
	jae     @@bodyWidenCarry
	dec     ss:ColumnsLeft
	inc     cl
@@bodyWidenCarry:
	add     ah, bh
	jae     @@bodyWiden
	dec     ss:RowsLeft
	jle     @@bodyOut
	cmp     ss:ColumnsLeft, 0
	jle     @@toTail
	dec     ss:SinCount
	jg      @@bodyStrip
	mov     bp, cx
	mov     cx, ss:SinStep
@@bodyNextPixel:
	inc     si
	add     ss:SinFraction, cl
	adc     ch, 0
	dec     ss:RunLength
	jle     @@bodyOut
	or      ch, ch
	je      @@bodyNextPixel
	mov     ss:SinCount, ch
	mov     cx, bp
	jmp     @@bodyPixel
@@bodyOut:
	ret
; Tail: strips cut at the far side.
@@tailRepeat:
	dec     ss:SinCount
@@tailFirst:
	cmp     ss:SinCount, 0
	jle     @@tailPixelDone
@@tailPixel:
	mov     al, [si]
@@tailStrip:
	push    di
	push    cx
	xor     ch, ch
	mov     bp, ss:LineX
	add     bp, cx
	stc
	sbb     bp, ss:ClipRight
	jle     @@tailFill
	sub     cx, bp
@@tailFill:
	rep     stosb
	pop     cx
	pop     di
	add     di, ss:RowPitch
@@tailSlide:
	add     dl, bl
	jae     @@tailSlideCarry
	dec     cl
	inc     di
	inc     ss:LineX
	mov     bp, ss:LineX
	cmp     bp, ss:ClipRight
	jg      @@finish
@@tailSlideCarry:
	add     ch, bh
	jae     @@tailSlide
@@tailWiden:
	add     dh, bl
	adc     cl, 0
	add     ah, bh
	jae     @@tailWiden
	dec     ss:RowsLeft
	jle     @@finish
	dec     ss:SinCount
	jg      @@tailStrip
@@tailPixelDone:
	mov     bp, cx
	mov     cx, ss:SinStep
@@tailNextPixel:
	inc     si
	add     ss:SinFraction, cl
	adc     ch, 0
	dec     ss:RunLength
	jle     @@finish
	or      ch, ch
	je      @@tailNextPixel
	mov     ss:SinCount, ch
	mov     cx, bp
	jmp     @@tailPixel
@@finish:
	ret
ClipRowsDownRight ENDP

; As ColumnsDownRight, clipped to the view. Pixels wholly before the clip box are
; skipped; the lead loop cuts strips at the near edge, the body draws whole
; strips while watching the room left, and the tail cuts them at the far
; edge. The body hands over to ColumnsDownRightInside once no edge is left.
ClipColumnsDownRight PROC NEAR
; Height of one scaled pixel.
	xor     ax, ax
	mov     al, ss:SinWhole
	mov     ch, ss:CosWhole
	xor     dx, dx
	or      ch, ch
	je      @@measured
@@measure:
	sub     dh, bh
	adc     ax, 0
	sub     dl, bl
	sbb     ch, 0
	jg      @@measure
@@measured:
	inc     ax
	mov     ss:SpanWidth, ax
	mov     ax, ss:LineX
	sub     ax, ss:ClipLeft
	jl      @@startOut
	jmp     @@startIn
@@startOut:
	or      ax, ax
	jns     @@startDistance
	neg     ax
@@startDistance:
	xor     dx, dx
	mov     dl, ah
	mov     ah, al
	xor     al, al
	sub     al, ss:XFraction
	sbb     ah, 0
	sbb     dx, 0
	xor     cx, cx
	mov     cl, bl
	div     cx
	or      dx, dx
	je      @@startSkip
	inc     ax
@@startSkip:
	push    ax
	mul     cx
	add     ss:XFraction, al
	adc     ah, 0
	adc     dx, 0
	mov     al, ah
	mov     ah, dl
	add     ss:LineX, ax
	add     ss:DriftX, ax
	pop     ax
	mov     cl, bh
	mul     cx
	add     ss:YFraction, al
	adc     ah, 0
	adc     dx, 0
	mov     al, ah
	mov     ah, dl
	add     ss:LineY, ax
	or      ss:ClipEdges, CLIP_LEFT
@@startIn:
	mov     ax, ss:ClipRight
	inc     ax
	sub     ax, ss:LineX
	jg      @@sideRoom
@@missed:
	ret
@@sideRoom:
	mov     ss:ColumnsLeft, ax
	mov     ax, ss:RunLength
	mul     ss:CosStep
	add     al, ss:CosFraction
	adc     ah, 0
	adc     dx, 0
	mov     al, ah
	mov     ah, dl
	add     ax, bp
	cmp     ax, ss:ClipLeft
	jl      @@missed
	cmp     ax, ss:ClipRight
	jle     @@sideDone
	or      ss:ClipEdges, CLIP_RIGHT
@@sideDone:
	mov     ax, ss:ClipBottom
	inc     ax
	sub     ax, ss:LineY
	mov     ss:RowsLeft, ax
	jg      @@endCheck
	add     ax, ss:SpanWidth
	jle     @@gone
	jg      @@endCut
@@endCheck:
	mov     ax, ss:RunLength
	mul     ss:SinStep
	add     al, ss:SinFraction
	adc     ah, 0
	adc     dx, 0
	mov     al, ah
	mov     ah, dl
	add     ax, di
	cmp     ax, ss:ClipTop
	jl      @@gone
	cmp     ax, ss:ClipBottom
	jl      @@edgesKnown
@@endCut:
	or      ss:ClipEdges, CLIP_BOTTOM
; Skip the source pixels wholly before the clip box.
@@edgesKnown:
	test    ss:ClipEdges, CLIP_LEFT or CLIP_TOP
	je      @@noSkip
	mov     ax, ss:DriftX
	xor     dx, dx
	mov     dl, ah
	mov     ah, al
	xor     al, al
	mov     al, ss:XFraction
	sub     al, ss:CosFraction
	sbb     ah, 0
	sbb     dx, 0
	mov     cx, ss:CosStep
	div     cx
	add     si, ax
	sub     ss:RunLength, ax
	jg      @@skipPixels
@@gone:
	ret
@@skipPixels:
	mul     cx
	add     ss:CosFraction, al
	adc     ah, 0
	adc     dx, 0
	add     ss:CosFraction, cl
	adc     ch, 0
	mov     al, ah
	mov     ah, dl
	add     al, ch
	adc     ah, 0
	sub     ax, ss:DriftX
	mov     ss:CosCount, al
	jmp     short @@address
@@noSkip:
	mov     cx, ss:CosStep
	add     ss:CosFraction, cl
	adc     ch, 0
	mov     ss:CosCount, ch
; Screen address of the first pixel, and the strip height.
@@address:
	mov     ax, ss:LineY
	shl     ax, 1
	add     ax, ss:ViewRows
	mov     di, ax
	mov     di, ss:[di]
	add     di, ss:LineX
	mov     ah, ss:YFraction
	mov     ch, ah
	mov     dl, ss:XFraction
	mov     dh, dl
	mov     cl, ss:SinWhole
	mov     al, ss:CosWhole
	or      al, al
	je      @@widthDone
@@width:
	sub     ah, bh
	adc     cl, 0
	sub     dh, bl
	sbb     al, 0
	jg      @@width
@@widthDone:
	mov     bp, ss:LineY
	cmp     bp, ss:ClipBottom
	jl      @@beforeEdge
	je      @@toTail
	sub     bp, ss:ClipBottom
	mov     al, ch
	xor     ch, ch
	sub     cx, bp
	mov     ch, al
	jg      @@pastEdge
	ret
@@pastEdge:
	mov     bp, ss:ClipBottom
	shl     bp, 1
	add     bp, ss:ViewRows
	mov     di, bp
	mov     di, ss:[di]
	add     di, ss:LineX
@@toTail:
	jmp     @@tail
@@beforeEdge:
	push    cx
	xor     ch, ch
	dec     cx
	sub     bp, cx
	pop     cx
	cmp     bp, ss:ClipTop
	jl      @@leadFirst
@@toBody:
	jmp     @@body
; Lead: strips cut at the near side.
@@leadFirst:
	cmp     ss:CosCount, 0
	je      @@leadPixelDone
@@leadPixel:
	mov     al, [si]
@@leadColumn:
	cmp     bp, ss:ClipTop
	jge     @@toBody
	push    cx
	push    di
	push    dx
	mov     dx, bp
	sub     dx, ss:ClipTop
	xor     ch, ch
	add     cx, dx
	pop     dx
@@leadPlot:
	mov     byte ptr es:[di], al
	sub     di, ss:RowPitch
	dec     cl
	jg      @@leadPlot
	pop     di
	pop     cx
	inc     di
	inc     ss:LineX
@@leadSlide:
	add     ch, bh
	jae     @@leadSlideCarry
	inc     cl
	add     di, ss:RowPitch
	dec     ss:RowsLeft
@@leadSlideCarry:
	add     dl, bl
	jae     @@leadSlide
@@leadWiden:
	add     ah, bh
	jae     @@leadWidenCarry
	inc     bp
	dec     cl
@@leadWidenCarry:
	add     dh, bl
	jae     @@leadWiden
	dec     ss:ColumnsLeft
	je      @@leadOut
	push    ax
	mov     ax, ss:LineX
	cmp     ax, ss:ClipRight
	pop     ax
	jg      @@leadOut
	dec     ss:CosCount
	jne     @@leadColumn
@@leadPixelDone:
	push    cx
	mov     cx, ss:CosStep
@@leadNextPixel:
	inc     si
	add     ss:CosFraction, cl
	adc     ch, 0
	dec     ss:RunLength
	jle     @@leadDone
	or      ch, ch
	jle     @@leadNextPixel
	mov     ss:CosCount, ch
	pop     cx
	jmp     @@leadPixel
@@leadDone:
	pop     cx
@@leadOut:
	jmp     @@finish
; Body: whole strips, watching the room left.
@@body:
	test    ss:ClipEdges, CLIP_RIGHT or CLIP_BOTTOM
	jne     @@bodyFirst
	jmp     ColumnsDownRightInside  ; no edge left: the plain loop finishes
@@bodyFirst:
	mov     bp, ss:RowPitch
	cmp     ss:CosCount, 0
	je      @@bodyPixelDone
@@bodyPixel:
	mov     al, [si]
@@bodyColumn:
	push    cx
	push    di
@@bodyPlot:
	mov     byte ptr es:[di], al
	sub     di, bp
	dec     cl
	jg      @@bodyPlot
	pop     di
	pop     cx
	inc     di
@@bodySlide:
	add     ch, bh
	jae     @@bodySlideCarry
	dec     ss:RowsLeft
	jle     @@bodySlideCarry
	inc     cl
	add     di, bp
@@bodySlideCarry:
	add     dl, bl
	jae     @@bodySlide
@@bodyWiden:
	add     ah, bh
	sbb     cl, 0
	add     dh, bl
	jae     @@bodyWiden
	inc     ss:LineX
	dec     ss:ColumnsLeft
	je      @@bodyOut
	cmp     ss:RowsLeft, 0
	jle     @@bodyOut
	dec     ss:CosCount
	jne     @@bodyColumn
@@bodyPixelDone:
	mov     bp, cx
	mov     cx, ss:CosStep
@@bodyNextPixel:
	inc     si
	add     ss:CosFraction, cl
	adc     ch, 0
	dec     ss:RunLength
	jle     @@bodyDone
	or      ch, ch
	jle     @@bodyNextPixel
	mov     ss:CosCount, ch
	mov     cx, bp
	mov     bp, ss:RowPitch
	jmp     @@bodyPixel
@@bodyDone:
	ret
@@bodyOut:
	test    ss:ClipEdges, CLIP_BOTTOM
	je      @@finish
	dec     ss:CosCount
; Tail: strips cut at the far side.
@@tail:
	mov     bp, ss:LineX
	cmp     bp, ss:ClipRight
	jg      @@finish
	cmp     ss:CosCount, 0
	jle     @@tailPixelDone
@@tailPixel:
	mov     al, [si]
@@tailColumn:
	push    cx
	push    di
@@tailPlot:
	mov     byte ptr es:[di], al
	sub     di, ss:RowPitch
	dec     cl
	jg      @@tailPlot
	pop     di
	pop     cx
	inc     di
	inc     bp
	cmp     bp, ss:ClipRight
	jg      @@finish
@@tailWiden:
	add     ah, bh
	sbb     cl, 0
	jle     @@finish
	add     dh, bl
	jae     @@tailWiden
	dec     ss:CosCount
	jne     @@tailColumn
@@tailPixelDone:
	push    cx
	mov     cx, ss:CosStep
@@tailNextPixel:
	inc     si
	add     ss:CosFraction, cl
	adc     ch, 0
	dec     ss:RunLength
	jle     @@tailDone
	or      ch, ch
	jle     @@tailNextPixel
	mov     ss:CosCount, ch
	pop     cx
	jmp     @@tailPixel
@@tailDone:
	pop     cx
@@finish:
	ret
ClipColumnsDownRight ENDP

; As StartUpLeft, for a frame the clip box cuts: keeps the first pixel's
; position in DrawX and DrawY, and skips ahead to the right edge when the span
; starts beyond it.
ClipStartUpLeft PROC NEAR
	add     ax, ss:FrameLeft
	mul     ss:CurrentScale
	mov     al, ah
	mov     ah, dl
	push    ax
	xor     cx, cx
	mov     cl, bh
	mul     cx
	mov     ss:YFraction, al
	mov     cx, ss:SinStep
	add     al, cl
	adc     ch, 0
	mov     ss:SinCount, ch
	mov     ss:SinFraction, al
	mov     al, ah
	mov     ah, dl
	neg     ax
	add     ax, ss:ScreenY
	mov     ss:LineY, ax
	mov     di, ax
	pop     ax
	xor     cx, cx
	mov     cl, bl
	mul     cx
	mov     ss:XFraction, al
	mov     ss:CosFraction, al
	mov     cl, al
	mov     al, ah
	mov     ah, dl
	neg     ax
	add     ax, ss:ScreenX
	mov     ss:LineX, ax
	mov     bp, ax
	mov     ss:DriftX, 0            ; nothing cut yet
	mov     ss:DriftY, 0
	mov     ss:ClipEdges, 0
	cmp     ax, ss:ClipLeft         ; starts past the far side?
	jge     @@inReach
	jmp     @@done
@@inReach:
	mov     ax, ss:LineX
	sub     ax, ss:ClipRight        ; rows above the clip box
	jle     @@run
	xor     dx, dx
	mov     dl, ah
	mov     ah, al
	xor     al, al
	sub     al, ss:XFraction
	sbb     ah, 0
	sbb     dx, 0
	xor     cx, cx
	mov     cl, bl
	div     cx                      ; source steps to reach the edge
	or      dx, dx
	je      @@advance
	inc     ax
@@advance:
	push    ax
	mul     cx
	add     ss:XFraction, al
	adc     ah, 0
	adc     dx, 0
	mov     al, ah
	mov     ah, dl
	sub     ss:LineX, ax
	add     ss:DriftX, ax
	pop     ax
	mov     cl, bh
	mul     cx
	add     ss:YFraction, al
	adc     ah, 0
	adc     dx, 0
	mov     al, ah
	mov     ah, dl
	sub     ss:LineY, ax
	add     ss:DriftY, ax
	or      ss:ClipEdges, CLIP_RIGHT
@@run:
	call    ss:StepProc
@@done:
	ret
ClipStartUpLeft ENDP

; As ClipRowsDownRight, going left and up.
ClipRowsUpLeft PROC NEAR
; Width of one scaled pixel across.
	xor     ax, ax
	mov     al, ss:CosWhole
	mov     ch, ss:SinWhole
	xor     dx, dx
	or      ch, ch
	je      @@measured
@@measure:
	sub     dl, bl
	adc     ax, 0
	sub     dh, bh
	sbb     ch, 0
	jg      @@measure
@@measured:
	inc     ax
	mov     ss:SpanWidth, ax
	mov     ax, ss:LineY
	sub     ax, ss:ClipBottom       ; ends before the near side?
	jle     @@startIn
	xor     dx, dx
	mov     dl, ah
	mov     ah, al
	xor     al, al
	sub     al, ss:YFraction
	sbb     ah, 0
	sbb     dx, 0
	xor     cx, cx
	mov     cl, bh
	div     cx
	or      dx, dx
	je      @@startSkip
	inc     ax
@@startSkip:
	push    ax
	mul     cx
	add     ss:YFraction, al
	adc     ah, 0
	adc     dx, 0
	mov     al, ah
	mov     ah, dl
	add     ss:DriftY, ax
	sub     ss:LineY, ax
	pop     ax
	xor     cx, cx
	mov     cl, bl
	mul     cx
	add     ss:XFraction, al
	adc     ah, 0
	adc     dx, 0
	mov     al, ah
	mov     ah, dl
	sub     ss:LineX, ax
	or      ss:ClipEdges, CLIP_BOTTOM
@@startIn:
	mov     ax, ss:LineX
	inc     ax
	sub     ax, ss:ClipLeft         ; room to the far side
	jg      @@sideRoom
	add     ax, ss:SpanWidth
	jg      @@endCut
	ret
@@sideRoom:
	mov     ss:ColumnsLeft, ax
	mov     ax, ss:RunLength
	mul     ss:CosStep
	add     al, ss:CosFraction
	adc     ah, 0
	adc     dx, 0
	mov     al, ah
	mov     ah, dl
	mov     dx, bp
	sub     dx, ax
	cmp     dx, ss:ClipRight
	jle     @@endOnScreen
	ret
@@endOnScreen:
	cmp     dx, ss:ClipLeft
	jge     @@sideDone
@@endCut:
	or      ss:ClipEdges, CLIP_LEFT
@@sideDone:
	mov     ax, ss:LineY
	inc     ax
	sub     ax, ss:ClipTop          ; room to the bottom
	jle     @@gone
	mov     ss:RowsLeft, ax
	mov     ax, ss:RunLength
	mul     ss:SinStep
	add     al, ss:SinFraction
	adc     ah, 0
	adc     dx, 0
	mov     al, ah
	mov     ah, dl
	neg     ax
	add     ax, di
	cmp     ax, ss:ClipBottom
	jg      @@gone
	cmp     ax, ss:ClipTop
	jge     @@edgesKnown
	or      ss:ClipEdges, CLIP_TOP
; Skip the source pixels wholly before the clip box.
@@edgesKnown:
	test    ss:ClipEdges, CLIP_RIGHT or CLIP_BOTTOM
	je      @@noSkip
	mov     ax, ss:DriftY
	or      ax, ax
	je      @@noSkip
	xor     dx, dx
	mov     dl, ah
	mov     ah, al
	xor     al, al
	mov     al, ss:YFraction
	sub     al, ss:SinFraction
	sbb     ah, 0
	sbb     dx, 0
	mov     cx, ss:SinStep
	div     cx                      ; whole source pixels to skip
	add     si, ax
	sub     ss:RunLength, ax
	jg      @@skipPixels
@@gone:
	ret
@@skipPixels:
	mul     cx
	add     ss:SinFraction, al
	adc     ah, 0
	adc     dx, 0
	add     ss:SinFraction, cl
	adc     ch, 0
	mov     al, ah
	mov     ah, dl
	add     al, ch
	adc     ah, 0
	sub     ax, ss:DriftY
	mov     ss:SinCount, al
	jmp     short @@address
@@noSkip:
	mov     cx, ss:SinStep
	add     ss:SinFraction, cl
	adc     ch, 0
	mov     ss:SinCount, ch
; Screen address of the first pixel, and the strip width.
@@address:
	mov     ax, ss:LineY
	shl     ax, 1
	add     ax, ss:ViewRows
	mov     di, ax
	mov     di, ss:[di]
	add     di, ss:LineX
	mov     ah, ss:YFraction
	mov     ch, ah
	mov     dl, ss:XFraction
	mov     dh, dl
	mov     cl, ss:CosWhole
	mov     al, ss:SinWhole
	or      al, al
	je      @@widthDone
@@width:
	sub     dh, bl
	adc     cl, 0
	sub     ah, bh
	sbb     al, 0
	jg      @@width
@@widthDone:
	test    ss:ClipEdges, CLIP_LEFT
	je      @@leadFirst
	mov     bp, ss:LineX
	sub     bp, ss:ClipLeft
	jg      @@leadFirst
	neg     bp
	add     di, bp
	sub     cx, bp
	jmp     @@tailPixel
; Lead: strips cut at the near side.
@@leadFirst:
	cmp     ss:SinCount, 0
	jle     @@leadPixelDone
@@leadPixel:
	mov     al, [si]
@@leadStrip:
	push    di
	push    dx
	push    cx
	xor     ch, ch
	mov     dx, ss:ClipRight
	inc     dx
	sub     dx, ss:LineX
	cmp     cx, dx
	jle     @@leadDone
	mov     cx, dx
	rep     stosb
	pop     cx
	pop     dx
	pop     di
	sub     di, ss:RowPitch
@@leadSlide:
	add     dl, bl
	jae     @@leadSlideCarry
	inc     cl
	dec     ss:LineX
	dec     di
	dec     ss:ColumnsLeft
@@leadSlideCarry:
	add     ch, bh
	jae     @@leadSlide
@@leadWiden:
	add     dh, bl
	sbb     cl, 0
	add     ah, bh
	jae     @@leadWiden
	dec     ss:RowsLeft
	jle     @@leadOut
	cmp     ss:ColumnsLeft, 0
	jle     @@toTail
	dec     ss:SinCount
	jg      @@leadStrip
@@leadPixelDone:
	mov     bp, cx
	mov     cx, ss:SinStep
@@leadNextPixel:
	inc     si
	add     ss:SinFraction, cl
	adc     ch, 0
	dec     ss:RunLength
	jle     @@leadOut
	or      ch, ch
	je      @@leadNextPixel
	mov     ss:SinCount, ch
	mov     cx, bp
	jmp     @@leadPixel
@@leadOut:
	ret
@@toTail:
	jmp     @@tailRepeat
; Body: whole strips, watching the room left.
@@leadDone:
	pop     cx
	pop     dx
	pop     di
	test    ss:ClipEdges, CLIP_LEFT or CLIP_TOP
	jne     @@bodyFirst
	jmp     RowsUpLeftInside        ; no edge left: the plain loop finishes
@@bodyFirst:
	cmp     ss:SinCount, 0
	jle     @@leadPixelDone
@@bodyPixel:
	mov     al, [si]
@@bodyStrip:
	mov     bp, cx
	xor     ch, ch
	rep     stosb
	mov     cx, bp
	sub     di, ss:RowPitch
	xor     ch, ch
	sub     di, cx
	mov     cx, bp
@@bodySlide:
	add     dl, bl
	jae     @@bodySlideCarry
	dec     ss:ColumnsLeft
	jle     @@bodySlideCarry
	inc     cl
	dec     di
@@bodySlideCarry:
	add     ch, bh
	jae     @@bodySlide
@@bodyWiden:
	add     dh, bl
	sbb     cl, 0
	jle     @@bodyOut
	add     ah, bh
	jae     @@bodyWiden
	dec     ss:RowsLeft
	jle     @@bodyOut
	cmp     ss:ColumnsLeft, 0
	jle     @@tailRepeat
	dec     ss:SinCount
	jg      @@bodyStrip
	mov     bp, cx
	mov     cx, ss:SinStep
@@bodyNextPixel:
	inc     si
	add     ss:SinFraction, cl
	adc     ch, 0
	dec     ss:RunLength
	jle     @@bodyOut
	or      ch, ch
	je      @@bodyNextPixel
	mov     ss:SinCount, ch
	mov     cx, bp
	jmp     @@bodyPixel
@@bodyOut:
	ret
; Tail: strips cut at the far side.
@@tailRepeat:
	dec     ss:SinCount
	jle     @@tailPixelDone
@@tailPixel:
	cmp     cl, 0
	jle     @@bodyOut
	mov     al, [si]
@@tailStrip:
	push    di
	push    cx
	xor     ch, ch
	rep     stosb
	pop     cx
	pop     di
	sub     di, ss:RowPitch
@@tailWiden:
	add     dh, bl
	sbb     cl, 0
	jle     @@finish
	add     ah, bh
	jae     @@tailWiden
	dec     ss:RowsLeft
	jle     @@finish
	dec     ss:SinCount
	jg      @@tailStrip
@@tailPixelDone:
	mov     bp, cx
	mov     cx, ss:SinStep
@@tailNextPixel:
	inc     si
	add     ss:SinFraction, cl
	adc     ch, 0
	dec     ss:RunLength
	jle     @@finish
	or      ch, ch
	je      @@tailNextPixel
	mov     ss:SinCount, ch
	mov     cx, bp
	jmp     @@tailPixel
@@finish:
	ret
ClipRowsUpLeft ENDP

; As ClipColumnsDownRight, going left and up.
ClipColumnsUpLeft PROC NEAR
; Height of one scaled pixel.
	xor     ax, ax
	mov     al, ss:SinWhole
	mov     ch, ss:CosWhole
	xor     dx, dx
	or      ch, ch
	je      @@measured
@@measure:
	add     dh, bh
	adc     ax, 0
	add     dl, bl
	sbb     ch, 0
	jg      @@measure
@@measured:
	inc     ax
	mov     ss:SpanWidth, ax
	mov     dx, ax
	mov     ax, ss:LineY
	sub     ax, ss:ClipBottom
	jle     @@startIn
	sub     ax, dx
	jle     @@startIn
	xor     dx, dx
	mov     dl, ah
	mov     ah, al
	xor     al, al
	sub     al, ss:YFraction
	sbb     ah, 0
	sbb     dx, 0
	xor     cx, cx
	mov     cl, bh
	div     cx
	or      dx, dx
	je      @@startSkip
	inc     ax
@@startSkip:
	push    ax
	mul     cx
	add     ss:YFraction, al
	adc     ah, 0
	adc     dx, 0
	mov     al, ah
	mov     ah, dl
	sub     ss:LineY, ax
	pop     ax
	xor     cx, cx
	mov     cl, bl
	mul     cx
	add     ss:XFraction, al
	adc     ah, 0
	adc     dx, 0
	mov     al, ah
	mov     ah, dl
	sub     ss:LineX, ax
	add     ss:DriftX, ax
	or      ss:ClipEdges, CLIP_BOTTOM
@@startIn:
	mov     ax, ss:LineX
	inc     ax
	sub     ax, ss:ClipLeft
	jg      @@sideRoom
@@missed:
	ret
@@sideRoom:
	mov     ss:ColumnsLeft, ax
	mov     ax, ss:RunLength
	mul     ss:CosStep
	add     al, ss:CosFraction
	adc     ah, 0
	adc     dx, 0
	mov     al, ah
	mov     ah, dl
	neg     ax
	add     ax, bp
	cmp     ax, ss:ClipRight
	jg      @@missed
	cmp     ax, ss:ClipLeft
	jge     @@sideDone
	or      ss:ClipEdges, CLIP_LEFT
@@sideDone:
	mov     ax, ss:LineY
	inc     ax
	sub     ax, ss:ClipTop
	jle     @@missed
	mov     ss:RowsLeft, ax
	mov     ax, ss:RunLength
	mul     ss:SinStep
	add     al, ss:SinFraction
	adc     ah, 0
	adc     dx, 0
	mov     al, ah
	mov     ah, dl
	add     ax, ss:SpanWidth
	neg     ax
	add     ax, di
	cmp     ax, ss:ClipBottom
	jg      @@gone
	cmp     ax, ss:ClipTop
	jge     @@edgesKnown
	or      ss:ClipEdges, CLIP_TOP
; Skip the source pixels wholly before the clip box.
@@edgesKnown:
	test    ss:ClipEdges, CLIP_RIGHT or CLIP_BOTTOM
	je      @@noSkip
	mov     ax, ss:DriftX
	xor     dx, dx
	mov     dl, ah
	mov     ah, al
	xor     al, al
	mov     al, ss:XFraction
	sub     al, ss:CosFraction
	sbb     ah, 0
	sbb     dx, 0
	mov     cx, ss:CosStep
	div     cx
	add     si, ax
	sub     ss:RunLength, ax
	jg      @@skipPixels
@@gone:
	ret
@@skipPixels:
	mul     cx
	add     ss:CosFraction, al
	adc     ah, 0
	adc     dx, 0
	add     ss:CosFraction, cl
	adc     ch, 0
	mov     al, ah
	mov     ah, dl
	add     al, ch
	adc     ah, 0
	sub     ax, ss:DriftX
	mov     ss:CosCount, al
	jmp     short @@address
@@noSkip:
	mov     cx, ss:CosStep
	add     ss:CosFraction, cl
	adc     ch, 0
	mov     ss:CosCount, ch
; Screen address of the first pixel, and the strip height.
@@address:
	mov     ax, ss:LineY
	shl     ax, 1
	add     ax, ss:ViewRows
	mov     di, ax
	mov     di, ss:[di]
	add     di, ss:LineX
	mov     cl, ss:SinWhole
	mov     al, ss:CosWhole
	mov     ah, ss:YFraction
	mov     ch, ah
	mov     dl, ss:XFraction
	mov     dh, dl
	or      al, al
	je      @@topCheck
@@width:
	add     ah, bh
	adc     cl, 0
	add     dh, bl
	sbb     al, 0
	jg      @@width
@@topCheck:
	test    ss:ClipEdges, CLIP_TOP
	je      @@widthDone
	push    cx
	xor     ch, ch
	inc     cx
	sub     ss:RowsLeft, cx
	pop     cx
	jge     @@widthDone
	jmp     @@tail
@@widthDone:
	mov     bp, ss:LineY
	cmp     bp, ss:ClipBottom
	jg      @@pastEdge
@@toBody:
	jmp     @@body
@@pastEdge:
	mov     bp, ss:ClipBottom
	shl     bp, 1
	add     bp, ss:ViewRows
	mov     di, bp
	mov     di, ss:[di]
	add     di, ss:LineX
	mov     bp, ss:LineY
; Lead: strips cut at the near side.
	cmp     ss:CosCount, 0
	je      @@leadPixelDone
@@leadPixel:
	mov     al, [si]
@@leadColumn:
	cmp     bp, ss:ClipBottom
	jle     @@toBody
	push    cx
	push    di
	push    dx
	mov     dx, bp
	sub     dx, ss:ClipBottom
	xor     ch, ch
	sub     cx, dx
	pop     dx
	jle     @@leadColumnDone
@@leadPlot:
	mov     byte ptr es:[di], al
	sub     di, ss:RowPitch
	dec     cl
	jg      @@leadPlot
@@leadColumnDone:
	pop     di
	pop     cx
	dec     di
@@leadSlide:
	add     ch, bh
	jae     @@leadSlideCarry
	dec     cl
	dec     ss:RowsLeft
	dec     bp
	cmp     bp, ss:ClipBottom
	jge     @@leadSlideCarry
	sub     di, ss:RowPitch
@@leadSlideCarry:
	add     dl, bl
	jae     @@leadSlide
@@leadWiden:
	add     ah, bh
	adc     cl, 0
	add     dh, bl
	jae     @@leadWiden
	dec     ss:ColumnsLeft
	je      @@leadOut
	dec     ss:CosCount
	jne     @@leadColumn
@@leadPixelDone:
	push    cx
	mov     cx, ss:CosStep
@@leadNextPixel:
	inc     si
	add     ss:CosFraction, cl
	adc     ch, 0
	dec     ss:RunLength
	jle     @@leadDone
	or      ch, ch
	jle     @@leadNextPixel
	mov     ss:CosCount, ch
	pop     cx
	jmp     @@leadPixel
@@leadDone:
	pop     cx
@@leadOut:
	jmp     @@finish
; Body: whole strips, watching the room left.
@@body:
	test    ss:ClipEdges, CLIP_LEFT or CLIP_TOP
	jne     @@bodyStart
	jmp     ColumnsUpLeftInside     ; no edge left: the plain loop finishes
@@bodyStart:
	mov     ss:LineY, bp
	mov     bp, ss:RowPitch
	cmp     ss:CosCount, 0
	je      @@bodyPixelDone
@@bodyPixel:
	mov     al, [si]
@@bodyColumn:
	push    cx
	push    di
@@bodyPlot:
	mov     byte ptr es:[di], al
	sub     di, bp
	dec     cl
	jg      @@bodyPlot
	pop     di
	pop     cx
	dec     di
@@bodySlide:
	add     ch, bh
	jae     @@bodySlideCarry
	dec     ss:RowsLeft
	dec     cl
	sub     di, bp
	dec     ss:LineY
@@bodySlideCarry:
	add     dl, bl
	jae     @@bodySlide
@@bodyWiden:
	add     ah, bh
	adc     cl, 0
	add     dh, bl
	jae     @@bodyWiden
	dec     ss:ColumnsLeft
	jle     @@bodyDone
	cmp     ss:RowsLeft, 0
	jle     @@bodyOut
	dec     ss:CosCount
	jne     @@bodyColumn
@@bodyPixelDone:
	mov     bp, cx
	mov     cx, ss:CosStep
@@bodyNextPixel:
	inc     si
	add     ss:CosFraction, cl
	adc     ch, 0
	dec     ss:RunLength
	jle     @@bodyDone
	or      ch, ch
	jle     @@bodyNextPixel
	mov     ss:CosCount, ch
	mov     cx, bp
	mov     bp, ss:RowPitch
	jmp     @@bodyPixel
@@bodyDone:
	ret
@@bodyOut:
	test    ss:ClipEdges, CLIP_TOP
	jne     @@tailRepeat
	ret
@@tailRepeat:
	dec     ss:CosCount
; Tail: strips cut at the far side.
@@tail:
	mov     bp, ss:LineY
	cmp     ss:CosCount, 0
	jle     @@tailPixelDone
@@tailPixel:
	mov     al, [si]
@@tailColumn:
	push    cx
	push    di
	push    bp
@@tailPlot:
	cmp     bp, ss:ClipTop
	jl      @@tailColumnDone
	mov     byte ptr es:[di], al
	sub     di, ss:RowPitch
	dec     bp
	dec     cl
	jg      @@tailPlot
@@tailColumnDone:
	pop     bp
	pop     di
	pop     cx
	dec     di
	dec     ss:ColumnsLeft
	jle     @@finish
@@tailSlide:
	add     ch, bh
	jae     @@tailSlideCarry
	dec     cl
	sub     di, ss:RowPitch
	dec     bp
	cmp     bp, ss:ClipTop
	jl      @@finish
@@tailSlideCarry:
	add     dl, bl
	jae     @@tailSlide
@@tailWiden:
	add     ah, bh
	adc     cl, 0
	add     dh, bl
	jae     @@tailWiden
	dec     ss:CosCount
	jne     @@tailColumn
@@tailPixelDone:
	push    cx
	mov     cx, ss:CosStep
@@tailNextPixel:
	inc     si
	add     ss:CosFraction, cl
	adc     ch, 0
	dec     ss:RunLength
	jle     @@tailDone
	or      ch, ch
	jle     @@tailNextPixel
	mov     ss:CosCount, ch
	pop     cx
	jmp     @@tailPixel
@@tailDone:
	pop     cx
@@finish:
	ret
ClipColumnsUpLeft ENDP

; Turned 91 to 179 degrees: spans run left and down, rows step left and up.
EnlargeTurnedBack PROC FAR
	call    far ptr PlaceEnlargedTurnedBack  ; carry: off the view
	jb      @@hidden
	call    far ptr FrameNeedsClip  ; carry: needs clipping
	jae     @@inside
	mov     ss:SpanProc, offset ClipStartDownLeft
	mov     ax, offset ClipRowsDownLeft
	cmp     bl, bh                  ; steeper than 45 degrees?
	jb      @@clipStep
	mov     ax, offset ClipColumnsDownLeft
@@clipStep:
	mov     ss:StepProc, ax
	jmp     WalkLeftUp
@@hidden:
	DRAW_EXIT
@@inside:
	mov     ss:SpanProc, offset StartDownLeft
	mov     ax, offset RowsDownLeft
	cmp     bl, bh
	jb      @@step
	mov     ax, offset ColumnsDownLeft
@@step:
	mov     ss:StepProc, ax
	jmp     WalkLeftUp
EnlargeTurnedBack ENDP

; Turned 91 to 179 degrees, mirrored: spans run right and up, rows step left
; and up.
EnlargeTurnedBackMirror PROC FAR
	call    far ptr PlaceEnlargedTurnedBackMirror  ; carry: off the view
	jb      @@hidden
	call    far ptr FrameNeedsClip  ; carry: needs clipping
	jae     @@inside
	mov     ss:SpanProc, offset ClipStartUpRight
	mov     ax, offset ClipRowsUpRight
	cmp     bl, bh                  ; steeper than 45 degrees?
	jb      @@clipStep
	mov     ax, offset ClipColumnsUpRight
@@clipStep:
	mov     ss:StepProc, ax
	jmp     WalkLeftUp
@@hidden:
	DRAW_EXIT
@@inside:
	mov     ss:SpanProc, offset StartUpRight
	mov     ax, offset RowsUpRight
	cmp     bl, bh
	jb      @@step
	mov     ax, offset ColumnsUpRight
@@step:
	mov     ss:StepProc, ax
	jmp     WalkLeftUp
EnlargeTurnedBackMirror ENDP

; Turned 91 to 179 degrees, flipped: spans run left and down, rows step right
; and down.
EnlargeTurnedBackFlip PROC FAR
	call    far ptr PlaceEnlargedTurnedBackFlip  ; carry: off the view
	jb      @@hidden
	call    far ptr FrameNeedsClip  ; carry: needs clipping
	jae     @@inside
	mov     ss:SpanProc, offset ClipStartDownLeft
	mov     ax, offset ClipRowsDownLeft
	cmp     bl, bh                  ; steeper than 45 degrees?
	jb      @@clipStep
	mov     ax, offset ClipColumnsDownLeft
@@clipStep:
	mov     ss:StepProc, ax
	jmp     WalkRightDown
@@hidden:
	DRAW_EXIT
@@inside:
	mov     ss:SpanProc, offset StartDownLeft
	mov     ax, offset RowsDownLeft
	cmp     bl, bh
	jb      @@step
	mov     ax, offset ColumnsDownLeft
@@step:
	mov     ss:StepProc, ax
	jmp     WalkRightDown
EnlargeTurnedBackFlip ENDP

; Turned 91 to 179 degrees, mirrored and flipped: spans run right and up, rows
; step right and down.
EnlargeTurnedBackBoth PROC FAR
	call    far ptr PlaceEnlargedTurnedBackBoth  ; carry: off the view
	jb      @@hidden
	call    far ptr FrameNeedsClip  ; carry: needs clipping
	jae     @@inside
	mov     ss:SpanProc, offset ClipStartUpRight
	mov     ax, offset ClipRowsUpRight
	cmp     bl, bh                  ; steeper than 45 degrees?
	jb      @@clipStep
	mov     ax, offset ClipColumnsUpRight
@@clipStep:
	mov     ss:StepProc, ax
	jmp     WalkRightDown
@@hidden:
	DRAW_EXIT
@@inside:
	mov     ss:SpanProc, offset StartUpRight
	mov     ax, offset RowsUpRight
	cmp     bl, bh
	jb      @@step
	mov     ax, offset ColumnsUpRight
@@step:
	mov     ss:StepProc, ax
	jmp     WalkRightDown
EnlargeTurnedBackBoth ENDP

; As StartDownRight, going left and down.
StartDownLeft PROC NEAR
	add     ax, ss:FrameLeft
	mul     ss:CurrentScale
	mov     al, ah
	mov     ah, dl
	push    ax
	xor     cx, cx
	mov     cl, bh
	mul     cx
	mov     ss:YFraction, al
	mov     ss:SinFraction, al
	mov     al, ah
	mov     ah, dl
	add     ax, ss:ScreenY
	shl     ax, 1
	add     ax, ss:ViewRows
	mov     di, ax
	mov     di, ss:[di]
	pop     ax
	mov     cl, bl
	mul     cx
	mov     ss:XFraction, al
	mov     ss:CosFraction, al
	mov     cl, al
	mov     al, ah
	mov     ah, dl
	mov     dx, ss:ScreenX
	sub     dx, ax
	add     di, dx
	mov     dl, cl
	jmp     ss:StepProc
StartDownLeft ENDP

; As RowsDownRight, going left and down.
RowsDownLeft PROC NEAR
	mov     cx, ss:SinStep
	add     ss:SinFraction, cl      ; rows this pixel covers
	adc     ch, 0
	mov     ss:SinCount, ch
	mov     cx, word ptr ss:CosWhole
	mov     al, ss:YFraction
	mov     ah, al
	mov     dh, dl
	or      ch, ch
	je      @@measured
@@measure:
	sub     dh, bl
	adc     cl, 0
	sub     ah, bh
	sbb     ch, 0
	jg      @@measure
@@measured:
	mov     ch, al
RowsDownLeftInside:
	cmp     ss:SinCount, 0
	jle     @@pixelDone
@@pixel:
	mov     al, [si]
@@strip:
	mov     bp, cx
	xor     ch, ch
	rep     stosb
	mov     cx, bp
	xor     ch, ch
	sub     di, cx
	mov     cx, bp
	add     di, ss:RowPitch
; Slide the strip by the turn, and widen it by the scale.
@@slide:
	add     dl, bl
	jae     @@slideCarry
	inc     cl
	dec     di
@@slideCarry:
	add     ch, bh
	jae     @@slide
@@widen:
	add     dh, bl
	sbb     cl, 0
	add     ah, bh
	jae     @@widen
	dec     ss:SinCount
	jne     @@strip
@@pixelDone:
	mov     bp, cx
	mov     cx, ss:SinStep
@@nextPixel:
	inc     si                      ; next source pixel
	add     ss:SinFraction, cl
	adc     ch, 0
	dec     ss:RunLength
	jle     @@done
	or      ch, ch
	je      @@nextPixel
	mov     ss:SinCount, ch
	mov     cx, bp
	jmp     @@pixel
@@done:
	ret
RowsDownLeft ENDP

; As ColumnsDownRight, going left and down.
ColumnsDownLeft PROC NEAR
	mov     cx, ss:CosStep
	mov     ax, cx
	add     ss:CosFraction, cl      ; columns this pixel covers
	adc     ch, 0
	mov     ss:CosCount, ch
	mov     cx, word ptr ss:CosWhole
	mov     ah, ss:YFraction
	xchg    ch, cl
	mov     dh, dl
	or      ch, ch
	je      @@measured
; Height of a strip: the whole steps of a row, spread by the turn.
@@measure:
	sub     ah, bh
	adc     cl, 0
	sub     dh, bl
	sbb     ch, 0
	jne     @@measure
@@measured:
	mov     ch, ss:YFraction
ColumnsDownLeftInside:
	mov     bp, ss:RowPitch
	mov     al, [si]
	cmp     ss:CosCount, 0
	je      @@pixelDone
@@pixel:
	mov     al, [si]
@@column:
	push    cx
	push    di
@@plot:
	mov     byte ptr es:[di], al
	sub     di, bp
	dec     cl
	jg      @@plot
	pop     di
	pop     cx
	dec     di
@@slide:
	add     ch, bh
	jae     @@slideCarry
	inc     cl
	add     di, bp
@@slideCarry:
	add     dl, bl
	jae     @@slide
@@widen:
	add     ah, bh
	sbb     cl, 0
	add     dh, bl
	jae     @@widen
	dec     ss:CosCount
	jne     @@column
@@pixelDone:
	mov     bp, cx
	mov     cx, ss:CosStep
@@nextPixel:
	inc     si                      ; next source pixel
	add     ss:CosFraction, cl
	adc     ch, 0
	dec     ss:RunLength
	jle     @@done
	or      ch, ch
	jle     @@nextPixel
	mov     ss:CosCount, ch
	mov     cx, bp
	mov     bp, ss:RowPitch
	jmp     @@pixel
@@done:
	ret
ColumnsDownLeft ENDP

; As StartDownRight, going right and up.
StartUpRight PROC NEAR
	add     ax, ss:FrameLeft
	mul     ss:CurrentScale
	mov     al, ah
	mov     ah, dl
	push    ax
	xor     cx, cx
	mov     cl, bh
	mul     cx
	mov     ss:YFraction, al
	mov     ss:SinFraction, al
	mov     al, ah
	mov     ah, dl
	neg     ax
	add     ax, ss:ScreenY
	shl     ax, 1
	add     ax, ss:ViewRows
	mov     di, ax
	mov     di, ss:[di]
	pop     ax
	mov     cl, bl
	mul     cx
	mov     ss:XFraction, al
	mov     ss:CosFraction, al
	mov     cl, al
	mov     al, ah
	mov     ah, dl
	add     ax, ss:ScreenX
	add     di, ax
	mov     dl, cl
	jmp     ss:StepProc
StartUpRight ENDP

; As RowsDownRight, going right and up.
RowsUpRight PROC NEAR
	mov     cx, ss:SinStep
	add     ss:SinFraction, cl      ; rows this pixel covers
	adc     ch, 0
	mov     ss:SinCount, ch
	mov     cx, word ptr ss:CosWhole
	mov     al, ss:YFraction
	mov     ah, al
	mov     dh, dl
	or      ch, ch
	je      @@measured
; Width of a strip: the whole steps of a row, spread by the turn.
@@measure:
	add     dh, bl
	adc     cl, 0
	add     ah, bh
	sbb     ch, 0
	jg      @@measure
@@measured:
	mov     ch, al
RowsUpRightInside:
	cmp     ss:SinCount, 0
	jle     @@pixelDone
@@pixel:
	mov     al, [si]
@@strip:
	mov     bp, cx
	xor     ch, ch
	rep     stosb
	mov     cx, bp
	xor     ch, ch
	sub     di, cx
	mov     cx, bp
	sub     di, ss:RowPitch
; Slide the strip by the turn, and widen it by the scale.
@@slide:
	add     dl, bl
	jae     @@slideCarry
	dec     cl
	inc     di
@@slideCarry:
	add     ch, bh
	jae     @@slide
@@widen:
	add     dh, bl
	adc     cl, 0
	add     ah, bh
	jae     @@widen
	dec     ss:SinCount
	jne     @@strip
@@pixelDone:
	mov     bp, cx
	mov     cx, ss:SinStep
@@nextPixel:
	inc     si                      ; next source pixel
	add     ss:SinFraction, cl
	adc     ch, 0
	dec     ss:RunLength
	jle     @@done
	or      ch, ch
	je      @@nextPixel
	mov     ss:SinCount, ch
	mov     cx, bp
	jmp     @@pixel
@@done:
	ret
RowsUpRight ENDP

; As ColumnsDownRight, going right and up.
ColumnsUpRight PROC NEAR
	mov     cx, ss:CosStep
	mov     ax, cx
	add     ss:CosFraction, cl      ; columns this pixel covers
	adc     ch, 0
	mov     ss:CosCount, ch
	mov     cx, word ptr ss:CosWhole
	mov     ah, ss:YFraction
	xchg    ch, cl
	mov     dh, dl
	or      ch, ch
	je      @@measured
@@measure:
	add     ah, bh
	adc     cl, 0
	add     dh, bl
	sbb     ch, 0
	jne     @@measure
@@measured:
	mov     ch, ss:YFraction
ColumnsUpRightInside:
	mov     bp, ss:RowPitch
	mov     al, [si]
	cmp     ss:CosCount, 0
	je      @@pixelDone
@@pixel:
	mov     al, [si]
@@column:
	push    cx
	push    di
@@plot:
	mov     byte ptr es:[di], al
	sub     di, bp
	dec     cl
	jg      @@plot
	pop     di
	pop     cx
	inc     di
@@slide:
	add     ch, bh
	jae     @@slideCarry
	dec     cl
	sub     di, bp
@@slideCarry:
	add     dl, bl
	jae     @@slide
@@widen:
	add     ah, bh
	adc     cl, 0
	add     dh, bl
	jae     @@widen
	dec     ss:CosCount
	jne     @@column
@@pixelDone:
	mov     bp, cx
	mov     cx, ss:CosStep
@@nextPixel:
	inc     si                      ; next source pixel
	add     ss:CosFraction, cl
	adc     ch, 0
	dec     ss:RunLength
	jle     @@done
	or      ch, ch
	jle     @@nextPixel
	mov     ss:CosCount, ch
	mov     cx, bp
	mov     bp, ss:RowPitch
	jmp     @@pixel
@@done:
	ret
ColumnsUpRight ENDP

; As WalkLeftDown, moving the pen left and up: before each row when the turn
; is steep, after it when shallow.
WalkLeftUp PROC FAR
	mov     ss:StepFraction, 0
; Add one scaled row to the fractions: CL and CH take the whole steps.
@@row:
	mov     dx, ss:SinStep
	add     byte ptr ss:StepFraction, dl  ; whole pixels the pen moves
	adc     dh, 0
	mov     cx, ss:CosStep
	add     byte ptr ss:StepFraction+1, cl
	adc     ch, 0
	mov     cl, dh
	mov     word ptr ss:CosWhole, cx
	cmp     bl, bh
	jae     @@moved
	xor     ax, ax
	mov     al, cl
	sub     ss:ScreenX, ax
	mov     al, ch
	sub     ss:ScreenY, ax
@@moved:
	or      cx, cx
	jne     @@span
	jmp     @@rowDone
@@span:
	mov     ax, [si].span_length    ; 0 ends the frame
	or      ax, ax
	jne     @@haveSpan
	jmp     @@done
@@haveSpan:
	mov     dx, [si].span_y
	cmp     dx, ss:SpanRow
	je      @@thisRow
	jl      @@skipSpan
	jmp     @@rowDone
@@skipSpan:
	add     si, SIZE SPANHDR
	shr     ax, 1                   ; odd: run-length encoded
	jb      @@skipRuns
	add     si, ax
	jmp     short @@skipped
@@skipRuns:
	push    di
	mov     di, ax
@@skipRun:
	lodsb
	inc     si
	shr     al, 1                   ; run byte: count*2 + 1 if a fill
	cbw
	jb      @@skippedRun
	add     si, ax
	dec     si
@@skippedRun:
	sub     di, ax
	jne     @@skipRun
	pop     di
@@skipped:
	jmp     @@span
@@thisRow:
	shr     ax, 1                   ; pixel count; carry: run-length encoded
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
; Unpack the runs into SpanBuffer and draw from there.
@@unpack:
	lodsb
	shr     al, 1                   ; run byte: count*2 + 1 if a fill
	cbw
	mov     cx, ax
	jb      @@unpackFill
	shr     cx, 1
	rep     movsw
	rcl     cx, 1                   ; and the odd byte
	rep     movsb
	sub     bx, ax
	jne     @@unpack
	je      @@unpacked
@@unpackFill:
	sub     bx, ax
	lodsb
	mov     ah, al
	shr     cx, 1
	rep     stosw
	rcl     cx, 1                   ; and the odd byte
	rep     stosb
	or      bx, bx
	jne     @@unpack
@@unpacked:
	pop     es
	pop     ax
	mov     bx, word ptr ss:ScaledCos  ; BL cos, BH sin
	push    si
	push    ds
	push    ss
	pop     ds
	mov     si, offset SpanBuffer
	jmp     short @@draw
@@raw:
	mov     di, ax
	mov     ax, [si].span_x
	add     si, SIZE SPANHDR
	add     di, si
	push    di
	push    ds
@@draw:
	call    ss:SpanProc
	pop     ds
	pop     si
	jmp     @@span
; Next source row.
@@rowDone:
	inc     ss:SpanRow
	dec     ss:FrameHeight
	je      @@done
	mov     cx, word ptr ss:CosWhole
	cmp     bl, bh
	jb      @@nextRow
	xor     ax, ax
	mov     al, cl
	sub     ss:ScreenX, ax
	mov     al, ch
	sub     ss:ScreenY, ax
@@nextRow:
	jmp     @@row
@@done:
	DRAW_EXIT
WalkLeftUp ENDP

; As WalkLeftDown, moving the pen right and down: before each row when the
; turn is shallow, after it when steep. A shallow turn starts a line higher.
WalkRightDown PROC FAR
	cmp     bl, bh
	jb      @@start
	dec     ss:ScreenY
@@start:
	mov     ss:StepFraction, 0
; Add one scaled row to the fractions: CL and CH take the whole steps.
@@row:
	mov     dx, ss:SinStep
	add     byte ptr ss:StepFraction, dl  ; whole pixels the pen moves
	adc     dh, 0
	mov     cx, ss:CosStep
	add     byte ptr ss:StepFraction+1, cl
	adc     ch, 0
	mov     cl, dh
	mov     word ptr ss:CosWhole, cx
	cmp     bl, bh
	jb      @@moved
	xor     ax, ax
	mov     al, cl
	add     ss:ScreenX, ax
	mov     al, ch
	add     ss:ScreenY, ax
@@moved:
	or      cx, cx
	jne     @@span
	jmp     @@rowDone
@@span:
	mov     ax, [si].span_length    ; 0 ends the frame
	or      ax, ax
	jne     @@haveSpan
	jmp     @@done
@@haveSpan:
	mov     dx, [si].span_y
	cmp     dx, ss:SpanRow
	je      @@thisRow
	jl      @@skipSpan
	jmp     @@rowDone
@@skipSpan:
	add     si, SIZE SPANHDR
	shr     ax, 1                   ; odd: run-length encoded
	jb      @@skipRuns
	add     si, ax
	jmp     short @@skipped
@@skipRuns:
	push    di
	mov     di, ax
@@skipRun:
	lodsb
	inc     si
	shr     al, 1                   ; run byte: count*2 + 1 if a fill
	cbw
	jb      @@skippedRun
	add     si, ax
	dec     si
@@skippedRun:
	sub     di, ax
	jne     @@skipRun
	pop     di
@@skipped:
	jmp     @@span
@@thisRow:
	shr     ax, 1                   ; pixel count; carry: run-length encoded
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
; Unpack the runs into SpanBuffer and draw from there.
@@unpack:
	lodsb
	shr     al, 1                   ; run byte: count*2 + 1 if a fill
	cbw
	mov     cx, ax
	jb      @@unpackFill
	shr     cx, 1
	rep     movsw
	rcl     cx, 1                   ; and the odd byte
	rep     movsb
	sub     bx, ax
	jne     @@unpack
	je      @@unpacked
@@unpackFill:
	sub     bx, ax
	lodsb
	mov     ah, al
	shr     cx, 1
	rep     stosw
	rcl     cx, 1                   ; and the odd byte
	rep     stosb
	or      bx, bx
	jne     @@unpack
@@unpacked:
	pop     es
	pop     ax
	mov     bx, word ptr ss:ScaledCos  ; BL cos, BH sin
	push    si
	push    ds
	push    ss
	pop     ds
	mov     si, offset SpanBuffer
	jmp     short @@draw
@@raw:
	mov     di, ax
	mov     ax, [si].span_x
	add     si, SIZE SPANHDR
	add     di, si
	push    di
	push    ds
@@draw:
	call    ss:SpanProc
	pop     ds
	pop     si
	jmp     @@span
; Next source row.
@@rowDone:
	inc     ss:SpanRow
	dec     ss:FrameHeight
	je      @@done
	mov     cx, word ptr ss:CosWhole
	cmp     bl, bh
	jae     @@nextRow
	xor     ax, ax
	mov     al, cl
	add     ss:ScreenX, ax
	mov     al, ch
	add     ss:ScreenY, ax
@@nextRow:
	jmp     @@row
@@done:
	DRAW_EXIT
WalkRightDown ENDP

; As StartDownLeft, for a frame the clip box cuts: keeps the first pixel's
; position in DrawX and DrawY, and skips ahead to the top edge when the span
; starts beyond it.
ClipStartDownLeft PROC NEAR
	add     ax, ss:FrameLeft
	mul     ss:CurrentScale
	mov     al, ah
	mov     ah, dl
	push    ax
	xor     cx, cx
	mov     cl, bh
	mul     cx
	mov     ss:YFraction, al
	mov     cx, ss:SinStep
	add     al, cl
	adc     ch, 0
	mov     ss:SinCount, ch
	mov     ss:SinFraction, al
	mov     al, ah
	mov     ah, dl
	add     ax, ss:ScreenY
	mov     ss:LineY, ax
	mov     di, ax
	pop     ax
	xor     cx, cx
	mov     cl, bl
	mul     cx
	mov     ss:XFraction, al
	mov     ss:CosFraction, al
	mov     cl, al
	mov     al, ah
	mov     ah, dl
	mov     dx, ss:ScreenX
	sub     dx, ax
	mov     ax, dx
	mov     ss:LineX, ax
	mov     bp, ax
	mov     ss:DriftX, 0            ; nothing cut yet
	mov     ss:DriftY, 0
	mov     ss:ClipEdges, 0
	cmp     ax, ss:ClipLeft         ; starts past the far side?
	jge     @@inReach
	ret
@@inReach:
	mov     ax, ss:LineY
	sub     ax, ss:ClipTop          ; rows above the clip box
	jge     @@run
	or      ax, ax
	jns     @@distance
	neg     ax
@@distance:
	xor     dx, dx
	mov     dl, ah
	mov     ah, al
	xor     al, al
	sub     al, ss:YFraction
	sbb     ah, 0
	sbb     dx, 0
	xor     cx, cx
	mov     cl, bh
	div     cx                      ; source steps to reach the edge
	or      dx, dx
	je      @@advance
	inc     ax
@@advance:
	push    ax
	mul     cx
	add     ss:YFraction, al
	adc     ah, 0
	adc     dx, 0
	mov     al, ah
	mov     ah, dl
	add     ss:DriftY, ax
	add     ss:LineY, ax
	pop     ax
	xor     cx, cx
	mov     cl, bl
	mul     cx
	add     ss:XFraction, al
	adc     ah, 0
	adc     dx, 0
	mov     al, ah
	mov     ah, dl
	sub     ss:LineX, ax
	add     ss:DriftX, ax
	or      ss:ClipEdges, CLIP_TOP
@@run:
	jmp     ss:StepProc
ClipStartDownLeft ENDP

; As ClipRowsDownRight, going left and down.
ClipRowsDownLeft PROC NEAR
	mov     ax, ss:LineX
	sub     ax, ss:ClipRight        ; ends before the near side?
	jle     @@startIn
	xor     dx, dx
	mov     dl, ah
	mov     ah, al
	xor     al, al
	sub     al, ss:XFraction
	sbb     ah, 0
	sbb     dx, 0
	xor     cx, cx
	mov     cl, bl
	div     cx
	or      dx, dx
	je      @@startSkip
	inc     ax
@@startSkip:
	push    ax
	mul     cx
	add     ss:XFraction, al
	adc     ah, 0
	adc     dx, 0
	mov     al, ah
	mov     ah, dl
	sub     ss:LineX, ax
	pop     ax
	mov     cl, bh
	mul     cx
	add     ss:YFraction, al
	adc     ah, 0
	adc     dx, 0
	mov     al, ah
	mov     ah, dl
	add     ss:LineY, ax
	add     ss:DriftY, ax
	or      ss:ClipEdges, CLIP_RIGHT
@@startIn:
	mov     ax, ss:LineX
	inc     ax
	sub     ax, ss:ClipLeft         ; room to the far side
	jg      @@sideRoom
	ret
@@sideRoom:
	mov     ss:ColumnsLeft, ax
	mov     ax, ss:RunLength
	mul     ss:CosStep
	add     al, ss:CosFraction
	adc     ah, 0
	adc     dx, 0
	mov     al, ah
	mov     ah, dl
	mov     dx, bp
	sub     dx, ax
	cmp     dx, ss:ClipRight
	jle     @@endOnScreen
	ret
@@endOnScreen:
	cmp     dx, ss:ClipLeft
	jge     @@sideDone
	or      ss:ClipEdges, CLIP_LEFT
@@sideDone:
	mov     ax, ss:ClipBottom
	inc     ax
	sub     ax, ss:LineY            ; room to the bottom
	mov     ss:RowsLeft, ax
	jle     @@gone
	mov     ax, ss:RunLength
	mul     ss:SinStep
	add     al, ss:SinFraction
	adc     ah, 0
	adc     dx, 0
	mov     al, ah
	mov     ah, dl
	add     ax, di
	cmp     ax, ss:ClipTop
	jl      @@gone
	cmp     ax, ss:ClipBottom
	jle     @@edgesKnown
	or      ss:ClipEdges, CLIP_BOTTOM
; Skip the source pixels wholly before the clip box.
@@edgesKnown:
	test    ss:ClipEdges, CLIP_TOP or CLIP_RIGHT
	je      @@noSkip
	mov     ax, ss:DriftY
	or      ax, ax
	je      @@gone
	xor     dx, dx
	mov     dl, ah
	mov     ah, al
	xor     al, al
	mov     al, ss:YFraction
	sub     al, ss:SinFraction
	sbb     ah, 0
	sbb     dx, 0
	mov     cx, ss:SinStep
	div     cx                      ; whole source pixels to skip
	add     si, ax
	sub     ss:RunLength, ax
	jg      @@skipPixels
@@gone:
	ret
@@skipPixels:
	mul     cx
	add     ss:SinFraction, al
	adc     ah, 0
	adc     dx, 0
	add     ss:SinFraction, cl
	adc     ch, 0
	mov     al, ah
	mov     ah, dl
	add     al, ch
	adc     ah, 0
	sub     ax, ss:DriftY
	mov     ss:SinCount, al
	jmp     short @@address
@@noSkip:
	mov     cx, ss:SinStep
	add     ss:SinFraction, cl
	adc     ch, 0
	mov     ss:SinCount, ch
; Screen address of the first pixel, and the strip width.
@@address:
	mov     ax, ss:LineY
	shl     ax, 1
	add     ax, ss:ViewRows
	mov     di, ax
	mov     di, ss:[di]
	add     di, ss:LineX
	mov     ah, ss:YFraction
	mov     ch, ah
	mov     dl, ss:XFraction
	mov     dh, dl
	mov     cl, ss:CosWhole
	mov     al, ss:SinWhole
	or      al, al
	je      @@leadFirst
@@width:
	sub     dh, bl
	adc     cl, 0
	sub     ah, bh
	sbb     al, 0
	jg      @@width
; Lead: strips cut at the near side.
@@leadFirst:
	cmp     ss:SinCount, 0
	jle     @@leadPixelDone
@@leadPixel:
	mov     al, [si]
@@leadStrip:
	push    di
	push    dx
	push    cx
	xor     ch, ch
	mov     dx, ss:ClipRight
	inc     dx
	sub     dx, ss:LineX
	cmp     cx, dx
	jle     @@leadDone
	mov     cx, dx
	rep     stosb
	pop     cx
	pop     dx
	pop     di
	add     di, ss:RowPitch
@@leadSlide:
	add     dl, bl
	jae     @@leadSlideCarry
	inc     cl
	dec     ss:LineX
	dec     di
	dec     ss:ColumnsLeft
@@leadSlideCarry:
	add     ch, bh
	jae     @@leadSlide
@@leadWiden:
	add     dh, bl
	sbb     cl, 0
	add     ah, bh
	jae     @@leadWiden
	dec     ss:RowsLeft
	jle     @@leadOut
	cmp     ss:ColumnsLeft, 0
	jle     @@toTail
	dec     ss:SinCount
	jg      @@leadStrip
@@leadPixelDone:
	mov     bp, cx
	mov     cx, ss:SinStep
@@leadNextPixel:
	inc     si
	add     ss:SinFraction, cl
	adc     ch, 0
	dec     ss:RunLength
	jle     @@leadOut
	or      ch, ch
	je      @@leadNextPixel
	mov     ss:SinCount, ch
	mov     cx, bp
	jmp     @@leadPixel
@@leadOut:
	ret
@@toTail:
	jmp     @@tailCheck
; Body: whole strips, watching the room left.
@@leadDone:
	pop     cx
	pop     dx
	pop     di
	test    ss:ClipEdges, CLIP_LEFT or CLIP_BOTTOM
	jne     @@bodyFirst
	jmp     RowsDownLeftInside      ; no edge left: the plain loop finishes
@@bodyFirst:
	cmp     ss:SinCount, 0
	jle     @@leadPixelDone
@@bodyPixel:
	mov     al, [si]
@@bodyStrip:
	mov     bp, cx
	xor     ch, ch
	rep     stosb
	mov     cx, bp
	add     di, ss:RowPitch
	xor     ch, ch
	sub     di, cx
	mov     cx, bp
@@bodySlide:
	add     dl, bl
	jae     @@bodySlideCarry
	dec     ss:ColumnsLeft
	jle     @@bodySlideCarry
	inc     cl
	dec     di
@@bodySlideCarry:
	add     ch, bh
	jae     @@bodySlide
@@bodyWiden:
	add     dh, bl
	sbb     cl, 0
	add     ah, bh
	jae     @@bodyWiden
	dec     ss:RowsLeft
	jle     @@bodyOut
	cmp     ss:ColumnsLeft, 0
	jle     @@tailCheck
	dec     ss:SinCount
	jg      @@bodyStrip
	mov     bp, cx
	mov     cx, ss:SinStep
@@bodyNextPixel:
	inc     si
	add     ss:SinFraction, cl
	adc     ch, 0
	dec     ss:RunLength
	jle     @@bodyOut
	or      ch, ch
	je      @@bodyNextPixel
	mov     ss:SinCount, ch
	mov     cx, bp
	jmp     @@bodyPixel
@@bodyOut:
	ret
@@tailCheck:
	or      cl, cl
	jle     @@bodyOut
; Tail: strips cut at the far side.
	dec     ss:SinCount
	jle     @@tailPixelDone
@@tailPixel:
	mov     al, [si]
@@tailStrip:
	push    di
	push    cx
	xor     ch, ch
	rep     stosb
	pop     cx
	pop     di
	add     di, ss:RowPitch
@@tailWiden:
	add     dh, bl
	sbb     cl, 0
	je      @@finish
	add     ah, bh
	jae     @@tailWiden
	dec     ss:RowsLeft
	jle     @@finish
	dec     ss:SinCount
	jg      @@tailStrip
@@tailPixelDone:
	mov     bp, cx
	mov     cx, ss:SinStep
@@tailNextPixel:
	inc     si
	add     ss:SinFraction, cl
	adc     ch, 0
	dec     ss:RunLength
	jle     @@finish
	or      ch, ch
	je      @@tailNextPixel
	mov     ss:SinCount, ch
	mov     cx, bp
	jmp     @@tailPixel
@@finish:
	ret
ClipRowsDownLeft ENDP

; As ClipColumnsDownRight, going left and down.
ClipColumnsDownLeft PROC NEAR
; Height of one scaled pixel.
	xor     ax, ax
	mov     ch, ss:CosWhole
	mov     al, ss:SinWhole
	xor     dx, dx
	or      ch, ch
	je      @@measured
@@measure:
	sub     dh, bh
	adc     ax, 0
	sub     dl, bl
	sbb     ch, 0
	jg      @@measure
@@measured:
	inc     ax
	mov     ss:SpanWidth, ax
	mov     ax, ss:LineX
	sub     ax, ss:ClipRight
	jg      @@startDistance
	jmp     @@startIn
@@startDistance:
	xor     dx, dx
	mov     dl, ah
	mov     ah, al
	xor     al, al
	sub     al, ss:XFraction
	sbb     ah, 0
	sbb     dx, 0
	xor     cx, cx
	mov     cl, bl
	div     cx
	or      dx, dx
	je      @@startSkip
	inc     ax
@@startSkip:
	push    ax
	mul     cx
	add     ss:XFraction, al
	adc     ah, 0
	adc     dx, 0
	mov     al, ah
	mov     ah, dl
	sub     ss:LineX, ax
	add     ss:DriftX, ax
	pop     ax
	mov     cl, bh
	mul     cx
	add     ss:YFraction, al
	adc     ah, 0
	adc     dx, 0
	mov     al, ah
	mov     ah, dl
	add     ss:LineY, ax
	or      ss:ClipEdges, CLIP_RIGHT
@@startIn:
	mov     ax, ss:LineX
	inc     ax
	sub     ax, ss:ClipLeft
	jg      @@sideRoom
@@missed:
	ret
@@sideRoom:
	mov     ss:ColumnsLeft, ax
	mov     ax, ss:RunLength
	mul     ss:CosStep
	add     al, ss:CosFraction
	adc     ah, 0
	adc     dx, 0
	mov     al, ah
	mov     ah, dl
	mov     dx, bp
	sub     dx, ax
	cmp     dx, ss:ClipRight
	jg      @@missed
	cmp     dx, ss:ClipLeft
	jge     @@sideDone
	or      ss:ClipEdges, CLIP_LEFT
@@sideDone:
	mov     ax, ss:ClipBottom
	inc     ax
	sub     ax, ss:LineY
	mov     ss:RowsLeft, ax
	jg      @@endCheck
	add     ax, ss:SpanWidth
	jle     @@gone
	jg      @@endCut
@@endCheck:
	mov     ax, ss:RunLength
	mul     ss:SinStep
	add     al, ss:SinFraction
	adc     ah, 0
	adc     dx, 0
	mov     al, ah
	mov     ah, dl
	add     ax, di
	cmp     ax, ss:ClipTop
	jl      @@gone
	cmp     ax, ss:ClipBottom
	jl      @@edgesKnown
@@endCut:
	or      ss:ClipEdges, CLIP_BOTTOM
; Skip the source pixels wholly before the clip box.
@@edgesKnown:
	test    ss:ClipEdges, CLIP_TOP or CLIP_RIGHT
	je      @@noSkip
	mov     ax, ss:DriftX
	xor     dx, dx
	mov     dl, ah
	mov     ah, al
	xor     al, al
	mov     al, ss:XFraction
	sub     al, ss:CosFraction
	sbb     ah, 0
	sbb     dx, 0
	mov     cx, ss:CosStep
	div     cx
	add     si, ax
	sub     ss:RunLength, ax
	jg      @@skipPixels
@@gone:
	ret
@@skipPixels:
	mul     cx
	add     ss:CosFraction, al
	adc     ah, 0
	adc     dx, 0
	add     ss:CosFraction, cl
	adc     ch, 0
	mov     al, ah
	mov     ah, dl
	add     al, ch
	adc     ah, 0
	sub     ax, ss:DriftX
	mov     ss:CosCount, al
	jmp     short @@address
@@noSkip:
	mov     cx, ss:CosStep
	add     ss:CosFraction, cl
	adc     ch, 0
	mov     ss:CosCount, ch
; Screen address of the first pixel, and the strip height.
@@address:
	mov     ax, ss:LineY
	shl     ax, 1
	add     ax, ss:ViewRows
	mov     di, ax
	mov     di, ss:[di]
	add     di, ss:LineX
	mov     cl, ss:SinWhole
	mov     al, ss:CosWhole
	mov     ah, ss:YFraction
	mov     ch, ah
	mov     dl, ss:XFraction
	mov     dh, dl
	or      al, al
	je      @@widthDone
@@width:
	sub     ah, bh
	adc     cl, 0
	sub     dh, bl
	sbb     al, 0
	jg      @@width
@@widthDone:
	mov     bp, ss:LineY
	cmp     bp, ss:ClipBottom
	jl      @@beforeEdge
	je      @@toTail
	sub     bp, ss:ClipBottom
	mov     al, ch
	xor     ch, ch
	sub     cx, bp
	mov     ch, al
	jg      @@pastEdge
	ret
@@pastEdge:
	mov     bp, ss:ClipBottom
	shl     bp, 1
	add     bp, ss:ViewRows
	mov     di, bp
	mov     di, ss:[di]
	add     di, ss:LineX
@@toTail:
	jmp     @@tail
@@beforeEdge:
	push    cx
	xor     ch, ch
	dec     cx
	sub     bp, cx
	pop     cx
	cmp     bp, ss:ClipTop
	jl      @@leadFirst
@@toBody:
	jmp     @@body
; Lead: strips cut at the near side.
@@leadFirst:
	cmp     ss:CosCount, 0
	je      @@leadPixelDone
@@leadPixel:
	mov     al, [si]
@@leadColumn:
	cmp     bp, ss:ClipTop
	jge     @@toBody
	push    cx
	push    di
	push    dx
	mov     dx, bp
	sub     dx, ss:ClipTop
	xor     ch, ch
	add     cx, dx
	pop     dx
@@leadPlot:
	mov     byte ptr es:[di], al
	sub     di, ss:RowPitch
	dec     cl
	jg      @@leadPlot
	pop     di
	pop     cx
	dec     di
	dec     ss:LineX
@@leadSlide:
	add     ch, bh
	jae     @@leadSlideCarry
	inc     cl
	add     di, ss:RowPitch
	dec     ss:RowsLeft
@@leadSlideCarry:
	add     dl, bl
	jae     @@leadSlide
@@leadWiden:
	add     ah, bh
	jae     @@leadWidenCarry
	inc     bp
	dec     cl
@@leadWidenCarry:
	add     dh, bl
	jae     @@leadWiden
	dec     ss:ColumnsLeft
	je      @@leadOut
	push    ax
	mov     ax, ss:LineX
	cmp     ax, ss:ClipLeft
	pop     ax
	jl      @@leadOut
	dec     ss:CosCount
	jne     @@leadColumn
@@leadPixelDone:
	push    cx
	mov     cx, ss:CosStep
@@leadNextPixel:
	inc     si
	add     ss:CosFraction, cl
	adc     ch, 0
	dec     ss:RunLength
	jle     @@leadDone
	or      ch, ch
	jle     @@leadNextPixel
	mov     ss:CosCount, ch
	pop     cx
	jmp     @@leadPixel
@@leadDone:
	pop     cx
@@leadOut:
	jmp     @@finish
; Body: whole strips, watching the room left.
@@body:
	test    ss:ClipEdges, CLIP_LEFT or CLIP_BOTTOM
	jne     @@bodyFirst
	jmp     ColumnsDownLeftInside   ; no edge left: the plain loop finishes
@@bodyFirst:
	mov     bp, ss:RowPitch
	cmp     ss:CosCount, 0
	je      @@bodyPixelDone
@@bodyPixel:
	mov     al, [si]
@@bodyColumn:
	push    cx
	push    di
@@bodyPlot:
	mov     byte ptr es:[di], al
	sub     di, bp
	dec     cl
	jg      @@bodyPlot
	pop     di
	pop     cx
	dec     di
@@bodySlide:
	add     ch, bh
	jae     @@bodySlideCarry
	dec     ss:RowsLeft
	jle     @@bodySlideCarry
	inc     cl
	add     di, bp
@@bodySlideCarry:
	add     dl, bl
	jae     @@bodySlide
@@bodyWiden:
	add     ah, bh
	sbb     cl, 0
	add     dh, bl
	jae     @@bodyWiden
	dec     ss:LineX
	dec     ss:ColumnsLeft
	jle     @@bodyDone
	cmp     ss:RowsLeft, 0
	jle     @@bodyOut
	dec     ss:CosCount
	jne     @@bodyColumn
@@bodyPixelDone:
	mov     bp, cx
	mov     cx, ss:CosStep
@@bodyNextPixel:
	inc     si
	add     ss:CosFraction, cl
	adc     ch, 0
	dec     ss:RunLength
	jle     @@bodyDone
	or      ch, ch
	jle     @@bodyNextPixel
	mov     ss:CosCount, ch
	mov     cx, bp
	mov     bp, ss:RowPitch
	jmp     @@bodyPixel
@@bodyDone:
	ret
@@bodyOut:
	test    ss:ClipEdges, CLIP_BOTTOM
	je      @@finish
	dec     ss:CosCount
; Tail: strips cut at the far side.
@@tail:
	mov     bp, ss:LineX
	cmp     bp, ss:ClipRight
	jg      @@finish
	cmp     ss:CosCount, 0
	jle     @@tailPixelDone
@@tailPixel:
	mov     al, [si]
@@tailColumn:
	push    cx
	push    di
@@tailPlot:
	mov     byte ptr es:[di], al
	sub     di, ss:RowPitch
	dec     cl
	jg      @@tailPlot
	pop     di
	pop     cx
	dec     di
	dec     bp
	cmp     bp, ss:ClipLeft
	jl      @@finish
@@tailWiden:
	add     ah, bh
	sbb     cl, 0
	jle     @@finish
	add     dh, bl
	jae     @@tailWiden
	dec     ss:ColumnsLeft
	jle     @@finish
	dec     ss:CosCount
	jne     @@tailColumn
@@tailPixelDone:
	push    cx
	mov     cx, ss:CosStep
@@tailNextPixel:
	inc     si
	add     ss:CosFraction, cl
	adc     ch, 0
	dec     ss:RunLength
	jle     @@tailDone
	or      ch, ch
	jle     @@tailNextPixel
	mov     ss:CosCount, ch
	pop     cx
	jmp     @@tailPixel
@@tailDone:
	pop     cx
@@finish:
	ret
ClipColumnsDownLeft ENDP

; As StartUpRight, for a frame the clip box cuts; the span routines do
; the skipping.
ClipStartUpRight PROC NEAR
	add     ax, ss:FrameLeft
	mul     ss:CurrentScale
	mov     al, ah
	mov     ah, dl
	push    ax
	xor     cx, cx
	mov     cl, bh
	mul     cx
	mov     ss:YFraction, al
	mov     cx, ss:SinStep
	add     al, cl
	adc     ch, 0
	mov     ss:SinCount, ch
	mov     ss:SinFraction, al
	mov     al, ah
	mov     ah, dl
	neg     ax
	add     ax, ss:ScreenY
	mov     ss:LineY, ax
	mov     di, ax
	pop     ax
	xor     cx, cx
	mov     cl, bl
	mul     cx
	mov     ss:XFraction, al
	mov     ss:CosFraction, al
	mov     cl, al
	mov     al, ah
	mov     ah, dl
	add     ax, ss:ScreenX
	mov     ss:LineX, ax
	mov     bp, ax
	mov     ss:DriftX, 0            ; nothing cut yet
	mov     ss:DriftY, 0
	mov     ss:ClipEdges, 0
	jmp     ss:StepProc
ClipStartUpRight ENDP

; As ClipRowsDownRight, going right and up; skips to the left and bottom
; edges itself.
ClipRowsUpRight PROC NEAR
; Width of one scaled pixel across.
	xor     ax, ax
	mov     al, ss:CosWhole
	mov     ch, ss:SinWhole
	xor     dx, dx
	or      ch, ch
	je      @@measured
@@measure:
	add     dl, bl
	adc     ax, 0
	add     dh, bh
	sbb     ch, 0
	jg      @@measure
@@measured:
	inc     ax
	mov     ss:SpanWidth, ax
	mov     dx, ax
	mov     ax, ss:LineX
	add     ax, dx
	sub     ax, ss:ClipLeft         ; ends before the near side?
	jl      @@startOut
	jmp     @@bottomCheck
@@startOut:
	or      ax, ax
	jns     @@startDistance
	neg     ax
@@startDistance:
	xor     dx, dx
	mov     dl, ah
	mov     ah, al
	xor     al, al
	sub     al, ss:XFraction
	sbb     ah, 0
	sbb     dx, 0
	xor     cx, cx
	mov     cl, bl
	div     cx
	or      dx, dx
	je      @@startSkip
	inc     ax
@@startSkip:
	push    ax
	mul     cx
	add     ss:XFraction, al
	adc     ah, 0
	adc     dx, 0
	mov     al, ah
	mov     ah, dl
	add     ss:LineX, ax
	pop     ax
	mov     cl, bh
	mul     cx
	add     ss:YFraction, al
	adc     ah, 0
	adc     dx, 0
	mov     al, ah
	mov     ah, dl
	sub     ss:LineY, ax
	add     ss:DriftY, ax
	or      ss:ClipEdges, CLIP_LEFT
@@bottomCheck:
	mov     ax, ss:LineY
	sub     ax, ss:ClipBottom
	jle     @@startIn
	xor     dx, dx
	mov     dl, ah
	mov     ah, al
	xor     al, al
	sub     al, ss:YFraction
	sbb     ah, 0
	sbb     dx, 0
	xor     cx, cx
	mov     cl, bh
	div     cx
	or      dx, dx
	je      @@bottomSkip
	inc     ax
@@bottomSkip:
	push    ax
	mul     cx
	add     ss:YFraction, al
	adc     ah, 0
	adc     dx, 0
	mov     al, ah
	mov     ah, dl
	add     ss:DriftY, ax
	sub     ss:LineY, ax
	pop     ax
	xor     cx, cx
	mov     cl, bl
	mul     cx
	add     ss:XFraction, al
	adc     ah, 0
	adc     dx, 0
	mov     al, ah
	mov     ah, dl
	add     ss:LineX, ax
	or      ss:ClipEdges, CLIP_BOTTOM
@@startIn:
	mov     ax, ss:ClipRight
	inc     ax
	sub     ax, ss:LineX            ; room to the far side
	jg      @@sideRoom
	ret
@@sideRoom:
	mov     ss:ColumnsLeft, ax
	mov     ax, ss:RunLength
	mul     ss:CosStep
	add     al, ss:CosFraction
	adc     ah, 0
	adc     dx, 0
	mov     al, ah
	mov     ah, dl
	add     ax, bp
	stc
	adc     al, ss:CosWhole
	adc     ah, 0
	cmp     ax, ss:ClipLeft
	jge     @@endInReach
	ret
@@endInReach:
	cmp     ax, ss:ClipRight
	jle     @@sideDone
	or      ss:ClipEdges, CLIP_RIGHT
@@sideDone:
	mov     ax, ss:LineY
	inc     ax
	sub     ax, ss:ClipTop          ; room to the bottom
	jle     @@gone
	mov     ss:RowsLeft, ax
	mov     ax, ss:RunLength
	mul     ss:SinStep
	add     al, ss:SinFraction
	adc     ah, 0
	adc     dx, 0
	mov     al, ah
	mov     ah, dl
	neg     ax
	add     ax, di
	cmp     ax, ss:ClipBottom
	jg      @@gone
	cmp     ax, ss:ClipTop
	jge     @@edgesKnown
	or      ss:ClipEdges, CLIP_TOP
; Skip the source pixels wholly before the clip box.
@@edgesKnown:
	test    ss:ClipEdges, CLIP_LEFT or CLIP_BOTTOM
	je      @@noSkip
	mov     ax, ss:DriftY
	or      ax, ax
	je      @@noSkip
	xor     dx, dx
	mov     dl, ah
	mov     ah, al
	xor     al, al
	mov     al, ss:YFraction
	sub     al, ss:SinFraction
	sbb     ah, 0
	sbb     dx, 0
	mov     cx, ss:SinStep
	div     cx                      ; whole source pixels to skip
	add     si, ax
	sub     ss:RunLength, ax
	jg      @@skipPixels
@@gone:
	ret
@@skipPixels:
	mul     cx
	add     ss:SinFraction, al
	adc     ah, 0
	adc     dx, 0
	add     ss:SinFraction, cl
	adc     ch, 0
	mov     al, ah
	mov     ah, dl
	add     al, ch
	adc     ah, 0
	sub     ax, ss:DriftY
	mov     ss:SinCount, al
	jmp     short @@address
@@noSkip:
	mov     cx, ss:SinStep
	add     ss:SinFraction, cl
	adc     ch, 0
	mov     ss:SinCount, ch
; Screen address of the first pixel, and the strip width.
@@address:
	mov     ax, ss:LineY
	shl     ax, 1
	add     ax, ss:ViewRows
	mov     di, ax
	mov     di, ss:[di]
	add     di, ss:LineX
	mov     ah, ss:YFraction
	mov     ch, ah
	mov     dl, ss:XFraction
	mov     dh, dl
	mov     cl, ss:CosWhole
	mov     al, ss:SinWhole
	or      al, al
	je      @@widthDone
@@width:
	add     dh, bl
	adc     cl, 0
	add     ah, bh
	sbb     al, 0
	jg      @@width
@@widthDone:
	push    cx
	xor     ch, ch
	sub     ss:ColumnsLeft, cx
	pop     cx
	jg      @@leadFirst
	jmp     @@tailFirst
; Lead: strips cut at the near side.
@@leadFirst:
	cmp     ss:SinCount, 0
	jle     @@leadPixelDone
@@leadPixel:
	mov     al, [si]
@@leadStrip:
	push    di
	push    dx
	push    cx
	xor     ch, ch
	mov     dx, ss:ClipLeft
	sub     dx, ss:LineX
	jle     @@leadDone
	sub     cx, dx
	jle     @@leadStripDone
	add     di, dx
	rep     stosb
@@leadStripDone:
	pop     cx
	pop     dx
	pop     di
	sub     di, ss:RowPitch
@@leadSlide:
	add     dl, bl
	jae     @@leadSlideCarry
	dec     cl
	inc     di
	inc     ss:LineX
@@leadSlideCarry:
	add     ch, bh
	jae     @@leadSlide
@@leadWiden:
	add     dh, bl
	jae     @@leadWidenCarry
	dec     ss:ColumnsLeft
	inc     cl
@@leadWidenCarry:
	add     ah, bh
	jae     @@leadWiden
	dec     ss:RowsLeft
	jle     @@leadOut
	cmp     ss:ColumnsLeft, 0
	jle     @@toTail
	dec     ss:SinCount
	jg      @@leadStrip
@@leadPixelDone:
	mov     bp, cx
	mov     cx, ss:SinStep
@@leadNextPixel:
	inc     si
	add     ss:SinFraction, cl
	adc     ch, 0
	dec     ss:RunLength
	jle     @@leadOut
	or      ch, ch
	je      @@leadNextPixel
	mov     ss:SinCount, ch
	mov     cx, bp
	jmp     @@leadPixel
@@leadOut:
	ret
@@toTail:
	jmp     @@tailRepeat
; Body: whole strips, watching the room left.
@@leadDone:
	pop     cx
	pop     dx
	pop     di
	test    ss:ClipEdges, CLIP_TOP or CLIP_RIGHT
	jne     @@bodyFirst
	mov     bp, ss:RunLength
	jmp     RowsUpRightInside       ; no edge left: the plain loop finishes
@@bodyFirst:
	cmp     ss:SinCount, 0
	jle     @@leadPixelDone
@@bodyPixel:
	mov     al, [si]
@@bodyStrip:
	mov     bp, cx
	xor     ch, ch
	rep     stosb
	mov     cx, bp
	sub     di, ss:RowPitch
	xor     ch, ch
	sub     di, cx
	mov     cx, bp
@@bodySlide:
	add     dl, bl
	jae     @@bodySlideCarry
	dec     cl
	inc     di
	inc     ss:LineX
@@bodySlideCarry:
	add     ch, bh
	jae     @@bodySlide
@@bodyWiden:
	add     dh, bl
	jae     @@bodyWidenCarry
	dec     ss:ColumnsLeft
	inc     cl
@@bodyWidenCarry:
	add     ah, bh
	jae     @@bodyWiden
	dec     ss:RowsLeft
	jle     @@bodyOut
	cmp     ss:ColumnsLeft, 0
	jle     @@toTail
	dec     ss:SinCount
	jg      @@bodyStrip
	mov     bp, cx
	mov     cx, ss:SinStep
@@bodyNextPixel:
	inc     si
	add     ss:SinFraction, cl
	adc     ch, 0
	dec     ss:RunLength
	jle     @@bodyOut
	or      ch, ch
	je      @@bodyNextPixel
	mov     ss:SinCount, ch
	mov     cx, bp
	jmp     @@bodyPixel
@@bodyOut:
	ret
; Tail: strips cut at the far side.
@@tailRepeat:
	dec     ss:SinCount
@@tailFirst:
	cmp     ss:SinCount, 0
	jle     @@tailPixelDone
@@tailPixel:
	mov     al, [si]
@@tailStrip:
	push    di
	push    cx
	xor     ch, ch
	mov     bp, ss:LineX
	add     bp, cx
	stc
	sbb     bp, ss:ClipRight
	jle     @@tailFill
	sub     cx, bp
@@tailFill:
	rep     stosb
	pop     cx
	pop     di
	sub     di, ss:RowPitch
@@tailSlide:
	add     dl, bl
	jae     @@tailSlideCarry
	dec     cl
	inc     di
	inc     ss:LineX
	mov     bp, ss:LineX
	cmp     bp, ss:ClipRight
	jg      @@finish
@@tailSlideCarry:
	add     ch, bh
	jae     @@tailSlide
@@tailWiden:
	add     dh, bl
	adc     cl, 0
	add     ah, bh
	jae     @@tailWiden
	dec     ss:RowsLeft
	jle     @@finish
	dec     ss:SinCount
	jg      @@tailStrip
@@tailPixelDone:
	mov     bp, cx
	mov     cx, ss:SinStep
@@tailNextPixel:
	inc     si
	add     ss:SinFraction, cl
	adc     ch, 0
	dec     ss:RunLength
	jle     @@finish
	or      ch, ch
	je      @@tailNextPixel
	mov     ss:SinCount, ch
	mov     cx, bp
	jmp     @@tailPixel
@@finish:
	ret
ClipRowsUpRight ENDP

; As ClipColumnsDownRight, going right and up; skips to the left and
; bottom edges itself.
ClipColumnsUpRight PROC NEAR
; Height of one scaled pixel.
	xor     ax, ax
	mov     ch, ss:CosWhole
	mov     al, ss:SinWhole
	xor     dx, dx
	or      ch, ch
	je      @@measured
@@measure:
	add     dh, bh
	adc     ax, 0
	add     dl, bl
	sbb     ch, 0
	jg      @@measure
@@measured:
	inc     ax
	mov     ss:SpanWidth, ax
	mov     ax, ss:LineX
	sub     ax, ss:ClipLeft
	jl      @@startOut
	jmp     @@bottomCheck
@@startOut:
	neg     ax
	xor     dx, dx
	mov     dl, ah
	mov     ah, al
	xor     al, al
	sub     al, ss:XFraction
	sbb     ah, 0
	sbb     dx, 0
	xor     cx, cx
	mov     cl, bl
	div     cx
	or      dx, dx
	je      @@startSkip
	inc     ax
@@startSkip:
	push    ax
	mul     cx
	add     ss:XFraction, al
	adc     ah, 0
	adc     dx, 0
	mov     al, ah
	mov     ah, dl
	add     ss:LineX, ax
	add     ss:DriftX, ax
	pop     ax
	mov     cl, bh
	mul     cx
	add     ss:YFraction, al
	adc     ah, 0
	adc     dx, 0
	mov     al, ah
	mov     ah, dl
	sub     ss:LineY, ax
	or      ss:ClipEdges, CLIP_LEFT
@@bottomCheck:
	mov     ax, ss:LineY
	sub     ax, ss:ClipBottom
	jle     @@startIn
	sub     ax, ss:SpanWidth
	jle     @@startIn
	xor     dx, dx
	mov     dl, ah
	mov     ah, al
	xor     al, al
	sub     al, ss:YFraction
	sbb     ah, 0
	sbb     dx, 0
	xor     cx, cx
	mov     cl, bh
	div     cx
	or      dx, dx
	je      @@bottomSkip
	inc     ax
@@bottomSkip:
	push    ax
	mul     cx
	add     ss:YFraction, al
	adc     ah, 0
	adc     dx, 0
	mov     al, ah
	mov     ah, dl
	sub     ss:LineY, ax
	pop     ax
	xor     cx, cx
	mov     cl, bl
	mul     cx
	add     ss:XFraction, al
	adc     ah, 0
	adc     dx, 0
	mov     al, ah
	mov     ah, dl
	add     ss:LineX, ax
	add     ss:DriftX, ax
	or      ss:ClipEdges, CLIP_BOTTOM
@@startIn:
	mov     ax, ss:ClipRight
	inc     ax
	sub     ax, ss:LineX
	jg      @@sideRoom
@@missed:
	ret
@@sideRoom:
	mov     ss:ColumnsLeft, ax
	mov     ax, ss:RunLength
	mul     ss:CosStep
	add     al, ss:CosFraction
	adc     ah, 0
	adc     dx, 0
	mov     al, ah
	mov     ah, dl
	add     ax, bp
	cmp     ax, ss:ClipLeft
	jl      @@missed
	cmp     ax, ss:ClipRight
	jle     @@sideDone
	or      ss:ClipEdges, CLIP_RIGHT
@@sideDone:
	mov     ax, ss:LineY
	inc     ax
	sub     ax, ss:ClipTop
	jle     @@missed
	mov     ss:RowsLeft, ax
	mov     ax, ss:RunLength
	mul     ss:SinStep
	add     al, ss:SinFraction
	adc     ah, 0
	adc     dx, 0
	mov     al, ah
	mov     ah, dl
	add     ax, ss:SpanWidth
	neg     ax
	add     ax, di
	cmp     ax, ss:ClipBottom
	jg      @@gone
	cmp     ax, ss:ClipTop
	jge     @@edgesKnown
	or      ss:ClipEdges, CLIP_TOP
; Skip the source pixels wholly before the clip box.
@@edgesKnown:
	test    ss:ClipEdges, CLIP_LEFT or CLIP_BOTTOM
	je      @@noSkip
	mov     ax, ss:DriftX
	xor     dx, dx
	mov     dl, ah
	mov     ah, al
	xor     al, al
	mov     al, ss:XFraction
	sub     al, ss:CosFraction
	sbb     ah, 0
	sbb     dx, 0
	mov     cx, ss:CosStep
	div     cx
	add     si, ax
	sub     ss:RunLength, ax
	jg      @@skipPixels
@@gone:
	ret
@@skipPixels:
	mul     cx
	add     ss:CosFraction, al
	adc     ah, 0
	adc     dx, 0
	add     ss:CosFraction, cl
	adc     ch, 0
	mov     al, ah
	mov     ah, dl
	add     al, ch
	adc     ah, 0
	sub     ax, ss:DriftX
	mov     ss:CosCount, al
	jmp     short @@address
@@noSkip:
	mov     cx, ss:CosStep
	add     ss:CosFraction, cl
	adc     ch, 0
	mov     ss:CosCount, ch
; Screen address of the first pixel, and the strip height.
@@address:
	mov     ax, ss:LineY
	shl     ax, 1
	add     ax, ss:ViewRows
	mov     di, ax
	mov     di, ss:[di]
	add     di, ss:LineX
	mov     cl, ss:SinWhole
	mov     al, ss:CosWhole
	mov     ah, ss:YFraction
	mov     ch, ah
	mov     dl, ss:XFraction
	mov     dh, dl
	or      al, al
	je      @@topCheck
@@width:
	add     ah, bh
	adc     cl, 0
	add     dh, bl
	sbb     al, 0
	jg      @@width
@@topCheck:
	test    ss:ClipEdges, CLIP_TOP
	je      @@widthDone
	push    cx
	xor     ch, ch
	inc     cx
	sub     ss:RowsLeft, cx
	pop     cx
	jge     @@widthDone
	jmp     @@tail
@@widthDone:
	mov     bp, ss:LineY
	cmp     bp, ss:ClipBottom
	jg      @@pastEdge
@@toBody:
	jmp     @@body
@@pastEdge:
	mov     bp, ss:ClipBottom
	shl     bp, 1
	add     bp, ss:ViewRows
	mov     di, bp
	mov     di, ss:[di]
	add     di, ss:LineX
	mov     bp, ss:LineY
; Lead: strips cut at the near side.
	cmp     ss:CosCount, 0
	je      @@leadPixelDone
@@leadPixel:
	mov     al, [si]
@@leadColumn:
	cmp     bp, ss:ClipBottom
	jle     @@toBody
	push    cx
	push    di
	push    dx
	mov     dx, bp
	sub     dx, ss:ClipBottom
	xor     ch, ch
	sub     cx, dx
	pop     dx
	jle     @@leadColumnDone
@@leadPlot:
	mov     byte ptr es:[di], al
	sub     di, ss:RowPitch
	dec     cl
	jg      @@leadPlot
@@leadColumnDone:
	pop     di
	pop     cx
	inc     di
@@leadSlide:
	add     ch, bh
	jae     @@leadSlideCarry
	dec     cl
	dec     ss:RowsLeft
	dec     bp
	cmp     bp, ss:ClipBottom
	jge     @@leadSlideCarry
	sub     di, ss:RowPitch
@@leadSlideCarry:
	add     dl, bl
	jae     @@leadSlide
@@leadWiden:
	add     ah, bh
	adc     cl, 0
	add     dh, bl
	jae     @@leadWiden
	dec     ss:ColumnsLeft
	je      @@leadOut
	dec     ss:CosCount
	jne     @@leadColumn
@@leadPixelDone:
	push    cx
	mov     cx, ss:CosStep
@@leadNextPixel:
	inc     si
	add     ss:CosFraction, cl
	adc     ch, 0
	dec     ss:RunLength
	jle     @@leadDone
	or      ch, ch
	jle     @@leadNextPixel
	mov     ss:CosCount, ch
	pop     cx
	jmp     @@leadPixel
@@leadDone:
	pop     cx
@@leadOut:
	jmp     @@finish
; Body: whole strips, watching the room left.
@@body:
	test    ss:ClipEdges, CLIP_TOP or CLIP_RIGHT
	jne     @@bodyStart
	jmp     ColumnsUpRightInside    ; no edge left: the plain loop finishes
@@bodyStart:
	mov     ss:LineY, bp
	mov     bp, ss:RowPitch
	cmp     ss:CosCount, 0
	je      @@bodyPixelDone
@@bodyPixel:
	mov     al, [si]
@@bodyColumn:
	push    cx
	push    di
@@bodyPlot:
	mov     byte ptr es:[di], al
	sub     di, bp
	dec     cl
	jg      @@bodyPlot
	pop     di
	pop     cx
	inc     di
@@bodySlide:
	add     ch, bh
	jae     @@bodySlideCarry
	dec     ss:RowsLeft
	dec     cl
	sub     di, bp
	dec     ss:LineY
@@bodySlideCarry:
	add     dl, bl
	jae     @@bodySlide
@@bodyWiden:
	add     ah, bh
	adc     cl, 0
	add     dh, bl
	jae     @@bodyWiden
	dec     ss:ColumnsLeft
	jle     @@bodyDone
	cmp     ss:RowsLeft, 0
	jle     @@bodyOut
	dec     ss:CosCount
	jne     @@bodyColumn
@@bodyPixelDone:
	mov     bp, cx
	mov     cx, ss:CosStep
@@bodyNextPixel:
	inc     si
	add     ss:CosFraction, cl
	adc     ch, 0
	dec     ss:RunLength
	jle     @@bodyDone
	or      ch, ch
	jle     @@bodyNextPixel
	mov     ss:CosCount, ch
	mov     cx, bp
	mov     bp, ss:RowPitch
	jmp     @@bodyPixel
@@bodyDone:
	ret
@@bodyOut:
	test    ss:ClipEdges, CLIP_TOP
	jne     @@tailRepeat
	ret
@@tailRepeat:
	dec     ss:CosCount
; Tail: strips cut at the far side.
@@tail:
	mov     bp, ss:LineY
	cmp     ss:CosCount, 0
	jle     @@tailPixelDone
@@tailPixel:
	mov     al, [si]
@@tailColumn:
	push    cx
	push    di
	push    bp
@@tailPlot:
	cmp     bp, ss:ClipTop
	jl      @@tailColumnDone
	mov     byte ptr es:[di], al
	sub     di, ss:RowPitch
	dec     bp
	dec     cl
	jg      @@tailPlot
@@tailColumnDone:
	pop     bp
	pop     di
	pop     cx
	inc     di
	dec     ss:ColumnsLeft
	jle     @@finish
@@tailSlide:
	add     ch, bh
	jae     @@tailSlideCarry
	dec     cl
	sub     di, ss:RowPitch
	dec     bp
	cmp     bp, ss:ClipTop
	jl      @@finish
@@tailSlideCarry:
	add     dl, bl
	jae     @@tailSlide
@@tailWiden:
	add     ah, bh
	adc     cl, 0
	add     dh, bl
	jae     @@tailWiden
	dec     ss:CosCount
	jne     @@tailColumn
@@tailPixelDone:
	push    cx
	mov     cx, ss:CosStep
@@tailNextPixel:
	inc     si
	add     ss:CosFraction, cl
	adc     ch, 0
	dec     ss:RunLength
	jle     @@tailDone
	or      ch, ch
	jle     @@tailNextPixel
	mov     ss:CosCount, ch
	pop     cx
	jmp     @@tailPixel
@@tailDone:
	pop     cx
@@finish:
	ret
ClipColumnsUpRight ENDP

	END
