; Black Gate U7.EXE, one module of resident segment 27 (file offsets 0x018764 to 0x018abe, 858 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.
; Segment 27 joins 25 modules' code as LOWLEVEL_TEXT, in link order and doubleword aligned.
; This one starts at 3A10h; its own empty segment 178 fixes its link position.
; Holds encoding screen pixels as a shape frame.

	.MODEL  MEDIUM
	.386
	LOCALS

	PUBLIC  _EncodeFrame

SCREEN_WIDTH    EQU 320

; A view: the segment its row addresses count from, the flat address of its
; table of row addresses, and its clip box, corners included.
VIEWREC STRUC
view_seg    dw  ?
view_rows   dd  ?
view_left   dw  ?
view_top    dw  ?
view_right  dw  ?
view_bottom dw  ?
VIEWREC ENDS

LOWLEVEL_TEXT SEGMENT DWORD PUBLIC USE16 'CODE'
	ASSUME  cs:LOWLEVEL_TEXT

; Encode a rectangle of a view into a shape frame: a four-word extent header, then
; one span per run of non-key pixels, each run-length packed when that saves enough.
; Writes to dest when it is non-zero; returns the byte total, or 0 past 64K.
_EncodeFrame PROC FAR
	ARG     view:WORD, left:WORD, top:WORD, right:WORD, bottom:WORD, hotX:WORD, hotY:WORD, dest:DWORD, \
		keyColor:BYTE, minGain:WORD
	LOCAL   unused1:DWORD, viewCopy:VIEWREC, hasDest:WORD, total:DWORD, unused2:WORD, col:WORD, \
		row:WORD, rawLen:WORD, packLen:WORD, unused3:DWORD, repeats:WORD, unused4:WORD, pixel:WORD, \
		rawPos:WORD, countAt:WORD, spanHead:DWORD, sink:WORD, packed:BYTE:2*SCREEN_WIDTH, \
		raw:BYTE:SCREEN_WIDTH = frame
	enter   frame, 0
	push    esi
	push    edi
	push    ds
	push    es
	pushf
	cld
	push    ds
	pop     es
	mov     si, view
	lea     di, viewCopy
	mov     cx, (SIZE VIEWREC) / 2
	rep     movsw
	push    0
	pop     es
	push    0
	pop     ds
	mov     word ptr hasDest, 0
	mov     dword ptr total, 0
	mov     edi, dest
	cmp     edi, 0
	je      short @@header
	mov     word ptr hasDest, 1
; header: extents right, left, above and below the hot spot
@@header:
	mov     ax, right
	sub     ax, hotX
	add     dword ptr total, 2
	test    word ptr hasDest, 1
	je      short @@leftExtent
	mov     [edi], ax
@@leftExtent:
	add     edi, 2
	mov     ax, hotX
	sub     ax, left
	add     dword ptr total, 2
	test    word ptr hasDest, 1
	je      short @@topExtent
	mov     [edi], ax
@@topExtent:
	add     edi, 2
	mov     ax, hotY
	sub     ax, top
	add     dword ptr total, 2
	test    word ptr hasDest, 1
	je      short @@bottomExtent
	mov     [edi], ax
@@bottomExtent:
	add     edi, 2
	mov     ax, bottom
	sub     ax, hotY
	add     dword ptr total, 2
	test    word ptr hasDest, 1
	je      short @@headerDone
	mov     [edi], ax
@@headerDone:
	add     edi, 2
	mov     ax, top
	mov     row, ax
	jmp     @@rowTest
@@nextRow:
	mov     ax, left
	mov     col, ax
	movzx   ebx, word ptr row
	shl     ebx, 2
	add     ebx, viewCopy.view_rows
	movsx   eax, word ptr col
	add     eax, [ebx]
	mov     esi, eax
	jmp     @@colTest
@@scanPixel:
	mov     al, [esi]
	cmp     al, keyColor
	je      @@keyPixel
	test    word ptr hasDest, 1
	je      short @@headAtSink
	mov     spanHead, edi
	jmp     short @@spanHeader
; counting only: the span length goes to a stack dummy
@@headAtSink:
	xor     eax, eax
	mov     ax, ss
	shl     eax, 4
	lea     ebx, sink
	add     eax, ebx
	mov     spanHead, eax
@@spanHeader:
	mov     ax, 0
	add     dword ptr total, 2
	test    word ptr hasDest, 1
	je      short @@spanX
	mov     [edi], ax
@@spanX:
	add     edi, 2
	mov     ax, col
	sub     ax, hotX
	add     dword ptr total, 2
	test    word ptr hasDest, 1
	je      short @@spanY
	mov     [edi], ax
@@spanY:
	add     edi, 2
	mov     ax, row
	sub     ax, hotY
	add     dword ptr total, 2
	test    word ptr hasDest, 1
	je      short @@gather
	mov     [edi], ax
; gather the span's pixels up to the next key pixel
@@gather:
	add     edi, 2
	mov     dx, col
	lea     bx, raw
	mov     cx, bx
	jmp     short @@gatherTest
@@copyPixel:
	mov     ss:[bx], al
	inc     esi
	inc     bx
	inc     dx
@@gatherTest:
	mov     al, [esi]
	cmp     al, keyColor
	je      short @@gathered
	cmp     dx, right
	jle     @@copyPixel
@@gathered:
	sub     bx, cx
	mov     rawLen, bx
	mov     packLen, bx
	mov     ax, minGain
	cmp     ax, 100
	jge     @@choose
	mov     word ptr packLen, 0
	lea     ax, raw
	mov     rawPos, ax
	mov     dx, 0
	jmp     @@packTest
; pack: count*2+1 then a byte repeats it; count*2 then that many literal bytes
@@nextRun:
	mov     word ptr repeats, 0
	mov     bx, rawPos
	inc     word ptr rawPos
	mov     al, ss:[bx]
	mov     byte ptr pixel, al
	inc     word ptr repeats
	inc     dx
	cmp     dx, rawLen
	jne     short @@notLast
	mov     ax, repeats
	shl     ax, 1
	mov     bx, packLen
	inc     word ptr packLen
	lea     cx, packed
	add     bx, cx
	mov     ss:[bx], al
	mov     al, byte ptr pixel
	inc     word ptr packLen
	inc     bx
	mov     ss:[bx], al
	jmp     @@choose
@@notLast:
	mov     bx, rawPos
	mov     al, ss:[bx]
	cmp     al, byte ptr pixel
	jne     short @@literal
@@repeatLoop:
	inc     word ptr repeats
	inc     dx
	inc     word ptr rawPos
	mov     bx, rawPos
	mov     al, ss:[bx]
	cmp     al, byte ptr pixel
	jne     short @@repeatDone
	cmp     dx, rawLen
	je      short @@repeatDone
	mov     ax, repeats
	cmp     ax, 127
	jne     @@repeatLoop
@@repeatDone:
	mov     ax, repeats
	shl     ax, 1
	or      ax, 1
	mov     cx, packLen
	inc     word ptr packLen
	lea     bx, packed
	add     bx, cx
	mov     ss:[bx], al
	mov     al, byte ptr pixel
	inc     word ptr packLen
	inc     bx
	mov     ss:[bx], al
	jmp     @@packTest
@@literal:
	lea     bx, packed
	add     bx, packLen
	inc     word ptr packLen
	mov     countAt, bx
@@literalLoop:
	mov     al, byte ptr pixel
	lea     bx, packed
	add     bx, packLen
	inc     word ptr packLen
	mov     ss:[bx], al
	mov     bx, rawPos
	mov     al, ss:[bx]
	cmp     al, byte ptr pixel
	je      short @@literalStop
	cmp     dx, rawLen
	je      short @@literalStop
	mov     ax, repeats
	cmp     ax, 127
	jne     short @@literalNext
@@literalStop:
	cmp     dx, rawLen
	je      short @@literalEnd
	mov     ax, repeats
	cmp     ax, 127
	jne     short @@literalBack
@@literalEnd:
	mov     ax, repeats
	shl     ax, 1
	mov     bx, countAt
	mov     ss:[bx], al
	jmp     short @@packTest
; give back the last pixel, it starts a repeat
@@literalBack:
	dec     dx
	dec     word ptr repeats
	dec     word ptr packLen
	dec     word ptr rawPos
	mov     ax, repeats
	shl     ax, 1
	mov     bx, countAt
	mov     ss:[bx], al
	jmp     short @@packTest
@@literalNext:
	mov     bx, rawPos
	mov     al, ss:[bx]
	mov     byte ptr pixel, al
	inc     word ptr repeats
	inc     dx
	inc     word ptr rawPos
	jmp     @@literalLoop
@@packTest:
	cmp     dx, rawLen
	jl      @@nextRun
; span length word is len*2, plus 1 when packed
@@choose:
	mov     cx, rawLen
	shl     cx, 1
	mov     ax, rawLen
	sub     ax, packLen
	jl      short @@useRaw
	mov     bx, 100
	mul     bx
	test    dx, 0FFFFh
	jne     short @@useRaw
	cmp     ax, minGain
	jl      short @@useRaw
	or      cx, 1
	mov     ebx, spanHead
	test    word ptr hasDest, 1
	je      short @@usePacked
	mov     [ebx], cx
@@usePacked:
	lea     ax, packed
	movzx   ecx, word ptr packLen
	jmp     short @@emitData
@@useRaw:
	mov     ebx, spanHead
	test    word ptr hasDest, 1
	je      short @@rawData
	mov     [ebx], cx
@@rawData:
	lea     ax, raw
	movzx   ecx, word ptr rawLen
@@emitData:
	test    word ptr hasDest, 1
	je      short @@skipData
	push    ecx
	push    esi
	movzx   esi, ax
	push    ss
	pop     ds
	rep     movs byte ptr es:[edi], byte ptr [esi]
	push    0
	pop     ds
	movzx   eax, bx
	sub     [edi], eax
	pop     esi
	pop     ecx
	jmp     @@spanDone
@@skipData:
	add     edi, ecx
@@spanDone:
	add     total, ecx
	mov     ax, col
	add     ax, rawLen
	mov     col, ax
	jmp     short @@colTest
@@keyPixel:
	inc     esi
	inc     word ptr col
@@colTest:
	mov     ax, col
	cmp     ax, right
	jle     @@scanPixel
	inc     word ptr row
@@rowTest:
	mov     ax, row
	cmp     ax, bottom
	jle     @@nextRow
	mov     ax, 0
	add     dword ptr total, 2
	test    word ptr hasDest, 1
	je      short @@finish
	mov     [edi], ax
; a zero word ends the frame
@@finish:
	add     edi, 2
	test    word ptr total+2, 0FFFFh
	je      short @@sizeOk
	mov     ax, 0
	jmp     short @@done
@@sizeOk:
	mov     ax, word ptr total
@@done:
	popf
	pop     es
	pop     ds
	pop     edi
	pop     esi
	leave
	ret
_EncodeFrame ENDP

LOWLEVEL_TEXT ENDS

	END
