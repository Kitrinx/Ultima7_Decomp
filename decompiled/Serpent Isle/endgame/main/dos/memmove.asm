; Serpent Isle ENDGAME.EXE, one module of resident segment 35 (file offsets 0x00d7ec to 0x00d89b, 175 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

	.MODEL  MEDIUM, PASCAL
	LOCALS
	JUMPS

	PUBLIC  MOVEFARMEMORY

	.CODE

downward    db  0                       ; set while copying from the top down

; Copy count bytes from src to dest, choosing the direction so that
; overlapping blocks survive.
MOVEFARMEMORY   PROC FAR dest:DWORD, src:DWORD, count:WORD
	USES    si, di, ds
	pushf
	cmp     count, 0
	je      @@done
	mov     cs:downward, 0
	lds     si, src                     ; normalize src into ds:si
	mov     ax, si
	mov     bx, ds
	mov     cl, 4
	shr     ax, cl
	add     bx, ax
	mov     ds, bx
	and     si, 0Fh
	mov     dx, bx
	les     di, dest                    ; and dest into es:di
	mov     ax, di
	mov     bx, es
	mov     cl, 4
	shr     ax, cl
	add     bx, ax
	mov     es, bx
	and     di, 0Fh
	cld
	mov     cx, count
	cmp     cx, 1
	je      @@oneByte
	jb      @@done
	cmp     dx, bx
	ja      @@upWords
	jb      @@downWords
	mov     ax, si
	sub     ax, di
	cmp     ax, -1                      ; src one byte below dest
	je      @@downBytes
	cmp     ax, 1                       ; or one byte above
	je      @@upBytes
	cmp     si, di
	ja      @@upWords
@@downWords:
	add     si, cx
	add     di, cx
	sub     si, 2
	sub     di, 2
	std
	inc     cs:downward
@@upWords:
	shr     cx, 1
	rep     movsw
	jnc     @@done
	cmp     cs:downward, 0
	je      @@oneByte
	inc     si                          ; the odd byte sits above the last word
	inc     di
@@oneByte:
	movsb
	jmp     short @@done
@@downBytes:
	std
	add     si, cx
	add     di, cx
	dec     si
	dec     di
@@upBytes:
	rep     movsb
@@done:
	popf
	ret
MOVEFARMEMORY   ENDP

	END
