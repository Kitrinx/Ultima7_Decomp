; Serpent Isle INTRO.EXE, resident segment 33 (file offsets 0x00e0f0 to 0x00e254, 356 bytes).
; Turbo Assembler 2.51 /mx /m2 rebuilds it byte for byte.

	.MODEL  MEDIUM, C
	LOCALS
	JUMPS

EMS_TAG         EQU     0C000h          ; segment of the first EMS page
PAGE_MASK       EQU     3Fh             ; offset bits above the low byte within a 16K page
IN_USE          EQU     1               ; low bit of a block's size
HEADER          EQU     4               ; every block starts with its size, a long
MAX_BLOCK       EQU     0FFFCh          ; most one block can hold

	PUBLIC  EmsAvailable, EmsLargestBlock
	EXTRN   MapEmsPointer:FAR
	EXTRN   EmsPointer:DWORD, EmsActive:WORD

	.CODE

; Returns the bytes in all free EMS blocks.
EmsAvailable        PROC FAR
	LOCAL   block:DWORD, saved:DWORD, total:DWORD
	push    si
	push    di
	cmp     EmsActive, 0
	je      @@fail
	les     ax, EmsPointer
	mov     word ptr saved, ax
	mov     word ptr saved+2, es
	xor     ax, ax
	mov     word ptr total, ax
	mov     word ptr total+2, ax
	mov     ax, EMS_TAG
	mov     word ptr block+2, ax
	push    ax
	xor     ax, ax
	mov     word ptr block, ax
	push    ax
	call    MapEmsPointer
	add     sp, 4
@@next:
	mov     es, dx
	mov     si, ax
	mov     ax, es:[si]
	mov     dx, es:[si+2]
	test    ax, IN_USE
	jne     @@step
	add     word ptr total, ax
	adc     word ptr total+2, dx
@@step:
	and     al, NOT IN_USE          ; on to the next block
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
	mov     word ptr block, ax
	mov     word ptr block+2, dx
	push    dx
	push    ax
	call    MapEmsPointer
	add     sp, 4
	or      dx, dx
	jne     @@next
	push    word ptr saved+2
	push    word ptr saved
	call    MapEmsPointer
	add     sp, 4
	mov     ax, word ptr total
	mov     dx, word ptr total+2
	pop     di
	pop     si
	ret
@@fail:
	xor     ax, ax
	xor     dx, dx
	pop     di
	pop     si
	ret
EmsAvailable        ENDP

; Returns the size of the largest free EMS block, at most MAX_BLOCK.
EmsLargestBlock     PROC FAR
	LOCAL   block:DWORD, saved:DWORD, largest:DWORD
	push    si
	push    di
	cmp     EmsActive, 0
	je      @@fail
	les     ax, EmsPointer
	mov     word ptr saved, ax
	mov     word ptr saved+2, es
	xor     ax, ax
	mov     word ptr largest, ax
	mov     word ptr largest+2, ax
	mov     ax, EMS_TAG
	mov     word ptr block+2, ax
	push    ax
	xor     ax, ax
	mov     word ptr block, ax
	push    ax
	call    MapEmsPointer
	add     sp, 4
@@next:
	mov     es, dx
	mov     si, ax
	mov     ax, es:[si]
	mov     dx, es:[si+2]
	test    ax, IN_USE
	jne     @@step
	cmp     dx, word ptr largest+2
	jb      @@step
	ja      @@larger
	cmp     ax, word ptr largest
	jbe     @@step
@@larger:
	mov     word ptr largest, ax
	mov     word ptr largest+2, dx
@@step:
	and     al, NOT IN_USE          ; on to the next block
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
	mov     word ptr block, ax
	mov     word ptr block+2, dx
	push    dx
	push    ax
	call    MapEmsPointer
	add     sp, 4
	or      dx, dx
	jne     @@next
	push    word ptr saved+2
	push    word ptr saved
	call    MapEmsPointer
	add     sp, 4
	mov     ax, word ptr largest
	mov     dx, word ptr largest+2
	cmp     ax, MAX_BLOCK
	ja      @@clamp
	or      dx, dx
	je      @@done
@@clamp:
	xor     dx, dx
	mov     ax, MAX_BLOCK
@@done:
	pop     di
	pop     si
	ret
@@fail:
	xor     ax, ax
	xor     dx, dx
	pop     di
	pop     si
	ret
EmsLargestBlock     ENDP

	END
