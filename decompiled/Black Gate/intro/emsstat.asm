; Black Gate INTRO.EXE, resident segment 49 (file offsets 0x01198a to 0x011aee, 356 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

	.MODEL  MEDIUM, C
	LOCALS

; Totals over the EMS heap: blocks end to end from the first page on, each
; after a DWORD header holding its size, bit 0 set while it is in use.

EMS_TAG         EQU     0C000h
BLOCK_USED      EQU     1
BLOCK_LIMIT     EQU     0FFFCh          ; the largest block AllocateEmsBlock gives

	PUBLIC  SumFreeEmsBlocks, FindLargestEmsBlock
	EXTRN   MapEmsPointer:FAR
	EXTRN   EmsActive:WORD, EmsPointer:DWORD

	.CODE

; Returns the bytes in all free EMS blocks.
SumFreeEmsBlocks    PROC FAR USES si di
	LOCAL   cursor:DWORD, saved:DWORD, total:DWORD
	cmp     EmsActive, 0
	jne     @@active
	jmp     @@none
@@active:
	les     ax, EmsPointer
	mov     word ptr saved, ax
	mov     word ptr saved+2, es
	xor     ax, ax
	mov     word ptr total, ax
	mov     word ptr total+2, ax
	mov     ax, EMS_TAG
	mov     word ptr cursor+2, ax
	push    ax
	xor     ax, ax
	mov     word ptr cursor, ax
	push    ax
	call    MapEmsPointer
	add     sp, 4
@@block:
	mov     es, dx
	mov     si, ax
	mov     ax, es:[si]
	mov     dx, es:[si+2]
	test    ax, BLOCK_USED
	jne     @@next
	add     word ptr total, ax
	adc     word ptr total+2, dx
@@next:
	and     al, NOT BLOCK_USED
	add     ax, 4
	adc     dx, 0
	add     ax, word ptr cursor
	adc     dx, 0
	mov     bh, ah                  ; carry the bits above the page into the segment
	and     ah, 3Fh
	shl     bh, 1
	rcl     dx, 1
	shl     bh, 1
	rcl     dx, 1
	add     dx, word ptr cursor+2
	mov     word ptr cursor, ax
	mov     word ptr cursor+2, dx
	push    dx
	push    ax
	call    MapEmsPointer
	add     sp, 4
	or      dx, dx
	jne     @@block
	push    word ptr saved+2
	push    word ptr saved
	call    MapEmsPointer
	add     sp, 4
	mov     ax, word ptr total
	mov     dx, word ptr total+2
	ret
@@none:
	xor     ax, ax
	xor     dx, dx
	ret
SumFreeEmsBlocks    ENDP

; Returns the size of the largest free EMS block, at most BLOCK_LIMIT.
FindLargestEmsBlock PROC FAR USES si di
	LOCAL   cursor:DWORD, saved:DWORD, largest:DWORD
	cmp     EmsActive, 0
	jne     @@active
	jmp     @@none
@@active:
	les     ax, EmsPointer
	mov     word ptr saved, ax
	mov     word ptr saved+2, es
	xor     ax, ax
	mov     word ptr largest, ax
	mov     word ptr largest+2, ax
	mov     ax, EMS_TAG
	mov     word ptr cursor+2, ax
	push    ax
	xor     ax, ax
	mov     word ptr cursor, ax
	push    ax
	call    MapEmsPointer
	add     sp, 4
@@block:
	mov     es, dx
	mov     si, ax
	mov     ax, es:[si]
	mov     dx, es:[si+2]
	test    ax, BLOCK_USED
	jne     @@next
	cmp     dx, word ptr largest+2
	jb      @@next
	ja      @@larger
	cmp     ax, word ptr largest
	jbe     @@next
@@larger:
	mov     word ptr largest, ax
	mov     word ptr largest+2, dx
@@next:
	and     al, NOT BLOCK_USED
	add     ax, 4
	adc     dx, 0
	add     ax, word ptr cursor
	adc     dx, 0
	mov     bh, ah                  ; carry the bits above the page into the segment
	and     ah, 3Fh
	shl     bh, 1
	rcl     dx, 1
	shl     bh, 1
	rcl     dx, 1
	add     dx, word ptr cursor+2
	mov     word ptr cursor, ax
	mov     word ptr cursor+2, dx
	push    dx
	push    ax
	call    MapEmsPointer
	add     sp, 4
	or      dx, dx
	jne     @@block
	push    word ptr saved+2
	push    word ptr saved
	call    MapEmsPointer
	add     sp, 4
	mov     ax, word ptr largest
	mov     dx, word ptr largest+2
	cmp     ax, BLOCK_LIMIT
	ja      @@cap
	or      dx, dx
	je      @@done
@@cap:
	xor     dx, dx
	mov     ax, BLOCK_LIMIT
@@done:
	ret
@@none:
	xor     ax, ax
	xor     dx, dx
	ret
FindLargestEmsBlock ENDP

	END
