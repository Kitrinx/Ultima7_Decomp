; Serpent Isle INTRO.EXE, resident segment 69 (file offsets 0x012596 to 0x01281d, 647 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

	.MODEL  MEDIUM, C
	LOCALS

	PUBLIC  DrawEmsFrame, DrawFrame

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

; DrawFrame for a shape that may sit in expanded memory: map it in first.
DrawEmsFrame    PROC FAR view:WORD, x:WORD, y:WORD, shape:DWORD, frameNum:WORD
	push    word ptr shape+2
	push    word ptr shape
	call    MapEmsPointer
	add     sp, 4
	or      dx, dx
	je      @@done
	mov     word ptr shape, ax
	mov     word ptr shape+2, dx
	push    frameNum
	push    word ptr shape+2
	push    word ptr shape
	push    y
	push    x
	push    view
	call    far ptr DrawFrame
	add     sp, 12
@@done:
	ret
DrawEmsFrame    ENDP

; Draw one frame of a run-length shape at x, y in a view, clipped to
; the view's clip box. Spans are rows of raw pixels or runs of copies and fills.
DrawFrame       PROC FAR view:WORD, x:WORD, y:WORD, shape:DWORD, frameNum:WORD
	LOCAL   overhang:WORD, unused:WORD:2, clipBottom:WORD, clipRight:WORD, clipTop:WORD, \
		clipLeft:WORD, rowTable:WORD, rowSeg:WORD, spanBuf:BYTE:SCREEN_WIDTH
	USES    si, di, ds
	cld
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
	mov     ax, [si].frame_right
	add     ax, x
	cmp     ax, clipRight
	jg      @@outsideBottom             ; the jumps below still hold these flags
	mov     ax, x
	sub     ax, [si].frame_left
	cmp     ax, clipLeft
	jl      @@outsideTop
	mov     ax, y
	sub     ax, [si].frame_top
	cmp     ax, clipTop
@@outsideTop:
	jl      @@clipped
	mov     ax, [si].frame_bottom
	add     ax, y
	cmp     ax, clipBottom
@@outsideBottom:
	jg      @@clipped
; whole frame inside the clip box: no per-span checks
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
	shr     cx, 1                       ; words, then the odd byte
	rep     movsw
	rcl     cx, 1
	rep     movsb
	jmp     @@fastSpan
; run byte: count*2 + 1 if a fill of one color
@@fastRle:
	mov     bx, cx
@@fastRun:
	lodsb
	shr     al, 1
	cbw
	mov     cx, ax
	jb      @@fastFill
	shr     cx, 1
	rep     movsw
	rcl     cx, 1
	rep     movsb
	sub     bx, ax
	jne     @@fastRun
	jmp     @@fastSpan
@@fastFill:
	sub     bx, ax
	lodsb
	mov     ah, al
	shr     cx, 1
	rep     stosw
	rcl     cx, 1
	rep     stosb
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
	jb      @@rleSpan
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
	jle     @@rawCopy
	sub     cx, dx
	shr     cx, 1
	rep     movsw
	rcl     cx, 1
	rep     movsb
	add     si, dx
	jmp     @@clipSpan
@@rawCopy:
	shr     cx, 1
	rep     movsw
	rcl     cx, 1
	rep     movsb
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
	shr     cx, 1
	rep     movsw
	rcl     cx, 1
	rep     movsb
	sub     bx, ax
	jne     @@rleRun
	jmp     @@clipSpan
@@rleFill:
	sub     bx, ax
	lodsb
	mov     ah, al
	shr     cx, 1
	rep     stosw
	rcl     cx, 1
	rep     stosb
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
; partly clipped runs: decode into spanBuf, then copy the visible part
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
	shr     cx, 1
	rep     movsw
	rcl     cx, 1
	rep     movsb
	sub     bx, ax
	jne     @@bufRun
	jmp     short @@bufCopy
@@bufFill:
	sub     bx, ax
	lodsb
	mov     ah, al
	shr     cx, 1
	rep     stosw
	rcl     cx, 1
	rep     stosb
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
	shr     cx, 1
	rep     movsw
	rcl     cx, 1
	rep     movsb
	add     si, ax
	pop     si
	pop     ds
	jmp     @@clipSpan
DrawFrame       ENDP

	END
