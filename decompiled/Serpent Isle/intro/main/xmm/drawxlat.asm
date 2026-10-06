; Serpent Isle INTRO.EXE, resident segment 67 (file offsets 0x0121f6 to 0x01250c, 790 bytes).
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

	.DATA

; No code reads these (DS:1582-177E): an identity table and a table of steps, zeros around them.
	db  34 dup (0)
identity    db  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
	db  16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31
	db  32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47
	db  48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63
	db  64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79
	db  80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95
	db  96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111
	db  112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127
	db  128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143
	db  144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159
	db  160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175
	db  176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191
	db  192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207
	db  208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223
	db  224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239
	db  240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255
	db  66 dup (0)
steps       dd  03h, 06h, 0Ch, 014h, 030h, 060h, 0B8h, 0110h
	dd  0240h, 0500h, 0CA0h, 01B00h, 03500h, 06000h, 0B400h, 012000h
	dd  020400h, 072000h, 090000h, 0140000h, 0300000h, 0420000h, 0D80000h, 01200000h
	dd  03880000h, 07200000h, 09000000h, 014000000h, 032800000h, 048000000h, 0A3000000h
	db  28 dup (0)

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
