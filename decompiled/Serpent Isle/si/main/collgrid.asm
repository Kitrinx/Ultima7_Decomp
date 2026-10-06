; Serpent Isle SI.EXE, resident segment 9 (file offsets 0x00f25e to 0x00f3f3, 405 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

	.MODEL  MEDIUM
	.386
	LOCALS

	PUBLIC  _AndCollisionCell, _OrCollisionCell, _GetCollisionCell
	PUBLIC  _SetCollisionCell, _OrCollisionBlock, _IsCollisionBlockSet

	.DATA
	EXTRN   _CollisionGrid:DWORD

; The grid is 512 bytes a row, a dword a cell, reached through a flat
; pointer with DS zeroed.

	.CODE

; cell[row][col] &= bits; returns the new cell in dx:ax
_AndCollisionCell   PROC FAR
	ARG row:WORD, col:WORD, bits:DWORD
	enter   0, 0
	push    esi
	push    ds
	mov esi, dword ptr _CollisionGrid
	xor eax, eax
	mov ds, ax
	mov ax, row
	shl eax, 9
	xor ebx, ebx
	mov bx, col
	shl ebx, 2
	add ebx, eax
	mov eax, [esi+ebx]
	mov edx, bits
	and eax, edx
	mov [esi+ebx], eax
	push    eax
	pop ax
	pop dx
	pop ds
	pop esi
	leave
	ret
_AndCollisionCell   ENDP

; cell[row][col] |= bits; returns the new cell in dx:ax
_OrCollisionCell    PROC FAR
	ARG row:WORD, col:WORD, bits:DWORD
	enter   0, 0
	push    esi
	push    ds
	mov esi, dword ptr _CollisionGrid
	xor eax, eax
	mov ds, ax
	mov ax, row
	shl eax, 9
	xor ebx, ebx
	mov bx, col
	shl ebx, 2
	add ebx, eax
	mov eax, [esi+ebx]
	mov edx, bits
	or  eax, edx
	mov [esi+ebx], eax
	push    eax
	pop ax
	pop dx
	pop ds
	pop esi
	leave
	ret
_OrCollisionCell    ENDP

; cell[row][col], in dx:ax
_GetCollisionCell   PROC FAR
	ARG row:WORD, col:WORD
	enter   0, 0
	push    esi
	push    ds
	mov esi, dword ptr _CollisionGrid
	xor eax, eax
	mov ds, ax
	mov ax, row
	shl eax, 9
	xor ebx, ebx
	mov bx, col
	shl ebx, 2
	add ebx, eax
	mov ax, [esi+ebx]
	mov dx, [esi+ebx+2]
	pop ds
	pop esi
	leave
	ret
_GetCollisionCell   ENDP

; cell[row][col] = bits
_SetCollisionCell   PROC FAR
	ARG row:WORD, col:WORD, bits:DWORD
	enter   0, 0
	push    esi
	push    ds
	mov esi, dword ptr _CollisionGrid
	xor eax, eax
	mov ds, ax
	mov ax, row
	shl eax, 9
	xor ebx, ebx
	mov bx, col
	shl ebx, 2
	add ebx, eax
	mov eax, bits
	mov [esi+ebx], eax
	pop ds
	pop esi
	leave
	ret
_SetCollisionCell   ENDP

; The OR of the cells in a block whose bottom-right corner is row, col,
; clipped at the grid edge. Both extents are one less than the size.
_OrCollisionBlock   PROC FAR
	ARG row:WORD, col:WORD, rows:WORD, cols:WORD
	enter   0, 0
	push    esi
	push    di
	push    ds
	mov ebx, dword ptr _CollisionGrid
	xor ecx, ecx
	mov ds, cx
	xor ax, ax
	xor dx, dx
@@row:
	mov esi, ebx    ; esi = &cell[row][col]
	mov cx, row
	shl cx, 7
	add cx, col
	shl cx, 2
	add esi, ecx
	mov cx, col
	mov di, cols
@@cell:
	or  ax, [esi]
	or  dx, [esi+2]
	dec cx
	js  @@nextRow   ; ran off the left edge
	sub esi, 4
	dec di
	jns @@cell
@@nextRow:
	dec row
	js  @@done      ; ran off the top edge
	dec rows
	jns @@row
@@done:
	pop ds
	pop di
	pop esi
	leave
	ret
_OrCollisionBlock   ENDP

; 1 when every cell of such a block shares a bit with bits, else 0
_IsCollisionBlockSet    PROC FAR
	ARG row:WORD, col:WORD, rows:WORD, cols:WORD, bits:DWORD
	enter   0, 0
	push    esi
	push    di
	push    ds
	mov ebx, dword ptr _CollisionGrid
	xor ecx, ecx
	mov ds, cx
@@row:
	mov esi, ebx
	mov cx, row
	shl cx, 7
	add cx, col
	shl cx, 2
	add esi, ecx
	mov cx, col
	mov di, cols
@@cell:
	mov ax, [esi]
	mov dx, [esi+2]
	and ax, word ptr bits
	and dx, word ptr bits+2
	or  ax, dx
	je  @@done      ; a clear cell: ax is 0
	dec cx
	js  @@nextRow
	sub esi, 4
	dec di
	jns @@cell
@@nextRow:
	dec row
	js  @@set
	dec rows
	jns @@row
@@set:
	mov ax, 1
@@done:
	pop ds
	pop di
	pop esi
	leave
	ret
_IsCollisionBlockSet    ENDP

	END
