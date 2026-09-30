; Black Gate ENDGAME.EXE, resident segment 33 (file offsets 0x00d6d8 to 0x00d8ea, 530 bytes).
; Turbo Assembler 2.51 /mx /m2 rebuilds it byte for byte.

	.MODEL  MEDIUM, C
	LOCALS
	JUMPS

EMS_TAG         EQU     0C000h          ; segment of the first EMS page
PAGE_MASK       EQU     3Fh             ; offset bits above the low byte within a 16K page
IN_USE          EQU     1               ; low bit of a block's size
HEADER          EQU     4               ; every block starts with its size, a long

	PUBLIC  AllocateEms, FreeEms, MeasureEmsBlock
	EXTRN   MapEmsPointer:FAR
	EXTRN   EmsPointer:DWORD, EmsActive:WORD

	.CODE

; Carve size bytes, rounded up to a long, out of the first free run of EMS
; blocks big enough to hold them. Returns an EMS pointer, or 0.
AllocateEms         PROC FAR size:DWORD
	LOCAL   block:DWORD, found:DWORD, saved:DWORD, need:DWORD, run:DWORD
	push    si
	push    di
	cmp     EmsActive, 0
	je      @@fail
	cmp     word ptr size+2, 0
	jne     @@fail
	cmp     word ptr size, -HEADER
	ja      @@fail
	add     word ptr size, HEADER-1
	and     word ptr size, NOT (HEADER-1)
	les     ax, EmsPointer
	mov     word ptr saved, ax
	mov     word ptr saved+2, es
	mov     ax, EMS_TAG
	mov     word ptr block+2, ax
	push    ax
	xor     ax, ax
	mov     word ptr block, ax
	push    ax
	call    MapEmsPointer
	add     sp, 4
	mov     es, dx
	mov     si, ax
	mov     ax, es:[si]
	mov     dx, es:[si+2]
	add     si, HEADER
@@check:
	test    ax, IN_USE
	je      @@free
@@used:
	call    NextEmsBlock
	jne     @@check
	jmp     @@fail
@@free:
	mov     bx, word ptr size       ; a block that would cross the page frame's end
	xor     cx, cx                  ; needs room for the next header past it
	add     si, bx
	jae     @@start
	sub     si, bx
	sub     bx, si
	add     bx, 4004h
	adc     cx, 0
@@start:
	mov     word ptr need, bx
	mov     word ptr need+2, cx
	mov     word ptr run, ax
	mov     word ptr run+2, dx
	mov     cx, word ptr block
	mov     word ptr found, cx
	mov     cx, word ptr block+2
	mov     word ptr found+2, cx
@@grow:
	test    ax, IN_USE
	jne     @@used
	mov     bx, word ptr run+2
	cmp     bx, word ptr need+2
	ja      @@enough
	jb      @@join
	mov     bx, word ptr run
	cmp     bx, word ptr need
	jae     @@enough
@@join:
	call    NextEmsBlock
	je      @@fail
	add     word ptr run, ax
	adc     word ptr run+2, dx
	add     word ptr run, HEADER
	adc     word ptr run+2, 0
	jmp     @@grow
@@enough:
	mov     dx, word ptr found
	mov     word ptr block, dx
	mov     dx, word ptr found+2
	mov     word ptr block+2, dx
	push    word ptr found+2
	push    word ptr found
	call    MapEmsPointer
	add     sp, 4
	mov     es, dx
	mov     si, ax
	mov     bx, word ptr size
	add     ax, HEADER
	add     ax, bx
	jae     @@split
	mov     ax, 3FFCh               ; pad the rest of the frame with a used block
	sub     ax, si
	mov     es:[si], ax
	xor     dx, dx
	mov     es:[si+2], dx
	add     si, HEADER
	sub     word ptr run, ax
	sbb     word ptr run+2, dx
	sub     word ptr run, HEADER
	sbb     word ptr run+2, 0
	call    NextEmsBlock
	je      @@fail
	push    cx
	mov     cx, word ptr block
	mov     word ptr found, cx
	mov     cx, word ptr block+2
	mov     word ptr found+2, cx
	pop     cx
	mov     bx, word ptr size
	sub     si, HEADER
@@split:
	sub     word ptr run, bx
	sbb     word ptr run+2, 0
	inc     bx                      ; size | IN_USE
	mov     es:[si], bx
	mov     word ptr es:[si+2], 0
	mov     ax, word ptr run
	or      ax, word ptr run+2
	je      @@restore
	mov     ax, bx
	xor     dx, dx
	call    NextEmsBlock
	je      @@fail
	mov     ax, word ptr run        ; the rest stays free
	sub     ax, HEADER
	mov     es:[si-4], ax
	mov     ax, word ptr run+2
	mov     es:[si-2], ax
@@restore:
	push    word ptr saved+2
	push    word ptr saved
	call    MapEmsPointer
	add     sp, 4
	mov     ax, word ptr found
	add     ax, HEADER
	mov     dx, word ptr found+2
	pop     di
	pop     si
	ret
@@fail:
	xor     ax, ax
	xor     dx, dx
	pop     di
	pop     si
	ret

; Step block, in the caller's frame, past the block whose size is in DX:AX
; and map the next one in. Returns its size in DX:AX and ES:SI past its
; header, or DX 0 (and ZF set) at the end of EMS.
NextEmsBlock:
	and     al, NOT IN_USE
	add     ax, HEADER
	adc     dx, 0
	add     ax, word ptr block
	adc     dx, 0
	mov     bh, ah                  ; carry the page bits into the segment
	and     ah, PAGE_MASK
	shl     bh, 1
	rcl     dx, 1
	shl     bh, 1
	rcl     dx, 1
	add     dx, word ptr block+2
	mov     word ptr block+2, dx
	mov     word ptr block, ax
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
	add     si, HEADER
@@end:
	retn
AllocateEms         ENDP

; Mark an EMS block free again.
FreeEms             PROC FAR p:DWORD
	LOCAL   saved:DWORD
	push    si
	push    di
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
	and     byte ptr es:[bx-HEADER], NOT IN_USE
	push    word ptr saved+2
	push    word ptr saved
	call    MapEmsPointer
	add     sp, 4
@@done:
	pop     di
	pop     si
	ret
FreeEms             ENDP

; Returns 0 for every EMS block.
MeasureEmsBlock     PROC FAR
	mov     ax, 0
	mov     dx, 0
	ret
MeasureEmsBlock     ENDP

	END
