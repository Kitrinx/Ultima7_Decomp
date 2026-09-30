; Black Gate INTRO.EXE, resident segment 48 (file offsets 0x011778 to 0x01198a, 530 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

	.MODEL  MEDIUM, C
	LOCALS

; The EMS heap: blocks laid end to end from the first page on. Each block starts
; with a DWORD header holding its size in bytes, bit 0 set while it is in use.
; An EMS pointer's segment carries EMS_TAG in its top bits and the page below
; them; its offset stays within the page.

EMS_TAG         EQU     0C000h
PAGE_END        EQU     3FFCh           ; last header position in a 16K page
BLOCK_USED      EQU     1

	PUBLIC  AllocateEmsBlock, ReleaseEmsBlock, MeasureEmsBlock
	EXTRN   MapEmsPointer:FAR
	EXTRN   EmsActive:WORD, EmsPointer:DWORD

	.CODE

; Returns an EMS block of at least size bytes, or 0. Free blocks in a row are
; joined until they are big enough; a block never crosses a page, so the part
; of a page too short for it is left as a free block of its own.
AllocateEmsBlock    PROC FAR USES si di, size:DWORD
	LOCAL   cursor:DWORD, start:DWORD, saved:DWORD, need:DWORD, run:DWORD
	cmp     EmsActive, 0
	jne     @@active
	jmp     @@fail
@@active:
	cmp     word ptr size+2, 0
	je      @@small
	jmp     @@fail
@@small:
	cmp     word ptr size, -4
	jbe     @@sized
	jmp     @@fail
@@sized:
	add     word ptr size, 3
	and     word ptr size, -4
	les     ax, EmsPointer
	mov     word ptr saved, ax
	mov     word ptr saved+2, es
	mov     ax, EMS_TAG
	mov     word ptr cursor+2, ax
	push    ax
	xor     ax, ax
	mov     word ptr cursor, ax
	push    ax
	call    MapEmsPointer
	add     sp, 4
	mov     es, dx
	mov     si, ax
	mov     ax, es:[si]
	mov     dx, es:[si+2]
	add     si, 4
@@findFree:
	test    ax, BLOCK_USED
	je      @@free
@@skip:
	call    NextEmsBlock
	jne     @@findFree
	jmp     @@fail
@@free:
	mov     bx, word ptr size
	xor     cx, cx
	add     si, bx
	jae     @@fits
	sub     si, bx                  ; runs past the page: count the rest of it too
	sub     bx, si
	add     bx, 4004h
	adc     cx, 0
@@fits:
	mov     word ptr need, bx
	mov     word ptr need+2, cx
	mov     word ptr run, ax
	mov     word ptr run+2, dx
	mov     cx, word ptr cursor
	mov     word ptr start, cx
	mov     cx, word ptr cursor+2
	mov     word ptr start+2, cx
@@grow:
	test    ax, BLOCK_USED
	jne     @@skip
	mov     bx, word ptr run+2
	cmp     bx, word ptr need+2
	ja      @@take
	jb      @@join
	mov     bx, word ptr run
	cmp     bx, word ptr need
	jae     @@take
@@join:
	call    NextEmsBlock
	jne     @@joined
	jmp     @@fail
@@joined:
	add     word ptr run, ax
	adc     word ptr run+2, dx
	add     word ptr run, 4
	adc     word ptr run+2, 0
	jmp     @@grow
@@take:
	mov     dx, word ptr start
	mov     word ptr cursor, dx
	mov     dx, word ptr start+2
	mov     word ptr cursor+2, dx
	push    word ptr start+2
	push    word ptr start
	call    MapEmsPointer
	add     sp, 4
	mov     es, dx
	mov     si, ax
	mov     bx, word ptr size
	add     ax, 4
	add     ax, bx
	jae     @@place
	mov     ax, PAGE_END            ; leave the end of the page free
	sub     ax, si
	mov     es:[si], ax
	xor     dx, dx
	mov     es:[si+2], dx
	add     si, 4
	sub     word ptr run, ax
	sbb     word ptr run+2, dx
	sub     word ptr run, 4
	sbb     word ptr run+2, 0
	call    NextEmsBlock
	je      @@fail
	push    cx
	mov     cx, word ptr cursor
	mov     word ptr start, cx
	mov     cx, word ptr cursor+2
	mov     word ptr start+2, cx
	pop     cx
	mov     bx, word ptr size
	sub     si, 4
@@place:
	sub     word ptr run, bx
	sbb     word ptr run+2, 0
	inc     bx
	mov     es:[si], bx
	mov     word ptr es:[si+2], 0
	mov     ax, word ptr run
	or      ax, word ptr run+2
	je      @@done
	mov     ax, bx                  ; what is left stays a free block
	xor     dx, dx
	call    NextEmsBlock
	je      @@fail
	mov     ax, word ptr run
	sub     ax, 4
	mov     es:[si-4], ax
	mov     ax, word ptr run+2
	mov     es:[si-2], ax
@@done:
	push    word ptr saved+2
	push    word ptr saved
	call    MapEmsPointer
	add     sp, 4
	mov     ax, word ptr start
	add     ax, 4
	mov     dx, word ptr start+2
	ret
@@fail:
	xor     ax, ax
	xor     dx, dx
	ret
AllocateEmsBlock    ENDP

; Steps the caller's cursor ([bp-4]) past the block whose header is in dx:ax
; and maps the next one. Returns its header in dx:ax and ES:SI past it, or
; zero flag set at the end of the heap.
NextEmsBlock        PROC NEAR
	and     al, NOT BLOCK_USED
	add     ax, 4
	adc     dx, 0
	add     ax, word ptr [bp-4]
	adc     dx, 0
	mov     bh, ah                  ; carry the bits above the page into the segment
	and     ah, 3Fh
	shl     bh, 1
	rcl     dx, 1
	shl     bh, 1
	rcl     dx, 1
	add     dx, word ptr [bp-2]
	mov     word ptr [bp-2], dx
	mov     word ptr [bp-4], ax
	push    dx
	push    ax
	call    MapEmsPointer
	add     sp, 4
	mov     es, dx
	mov     si, ax
	or      dx, dx
	je      @@end
	mov     ax, es:[si]
	mov     dx, es:[si+2]
	add     si, 4
@@end:
	ret
NextEmsBlock        ENDP

; Marks the block at p free.
ReleaseEmsBlock     PROC FAR USES si di, p:DWORD
	LOCAL   saved:DWORD
	cmp     EmsActive, 0
	je      @@done
	mov     ax, word ptr p+2
	and     ax, EMS_TAG
	cmp     ax, EMS_TAG
	jne     @@done
	les     ax, EmsPointer
	mov     word ptr saved, ax
	mov     word ptr saved+2, es
	push    word ptr p+2
	push    word ptr p
	call    MapEmsPointer
	add     sp, 4
	mov     es, dx
	mov     bx, ax
	and     byte ptr es:[bx-4], NOT BLOCK_USED
	push    word ptr saved+2
	push    word ptr saved
	call    MapEmsPointer
	add     sp, 4
@@done:
	ret
ReleaseEmsBlock     ENDP

; Returns 0 for every EMS block.
MeasureEmsBlock     PROC FAR
	mov     ax, 0
	mov     dx, 0
	ret
MeasureEmsBlock     ENDP

	END
