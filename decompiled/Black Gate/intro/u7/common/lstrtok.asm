; Black Gate U7.EXE, resident segment 28 (file offsets 0x018b52 to 0x018be2, 144 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.
; Original folder unknown.

	.MODEL  MEDIUM

	PUBLIC  _NextToken

	.CODE

; Tokenizer (text, separators, &cursor): skips leading separators, returns the token start
; and ends it with a zero, leaving the cursor after it. With text 0 it continues from the
; cursor. Returns 0 when nothing is left.
_NextToken PROC FAR
	ARG     text:WORD, separators:WORD, cursor:WORD
	push    bp
	mov     bp, sp
	push    si
	push    di
	mov     di, text
	mov     dx, separators
	or      di, di
	je      skip_loop
	mov     bx, cursor
	mov     word ptr [bx], di
	jmp     skip_loop
skip_next:
	mov     si, dx
	jmp     skip_test
skip_scan:
	mov     al, byte ptr [si]
	mov     di, cursor
	mov     bx, word ptr [di]
	cmp     al, byte ptr [bx]
	je      skip_found
	inc     si
skip_test:
	cmp     byte ptr [si], 0
	jne     skip_scan
skip_found:
	cmp     byte ptr [si], 0
	je      skipped
	mov     di, cursor
	inc     word ptr [di]
skip_loop:
	mov     di, cursor
	mov     bx, word ptr [di]
	cmp     byte ptr [bx], 0
	jne     skip_next
skipped:
	mov     di, cursor
	mov     bx, word ptr [di]
	cmp     byte ptr [bx], 0
	jne     token
	xor     ax, ax
	jmp     done
token:
	mov     di, cursor
	mov     ax, word ptr [di]
	mov     cx, ax
	jmp     scan_loop
scan_next:
	mov     si, dx
	jmp     sep_test
scan_sep:
	mov     al, byte ptr [si]
	mov     di, cursor
	mov     bx, word ptr [di]
	cmp     al, byte ptr [bx]
	jne     sep_next
	mov     byte ptr [bx], 0
	mov     di, cursor
	inc     word ptr [di]
	jmp     found_end
sep_next:
	inc     si
sep_test:
	cmp     byte ptr [si], 0
	jne     scan_sep
	mov     di, cursor
	inc     word ptr [di]
scan_loop:
	mov     di, cursor
	mov     bx, word ptr [di]
	cmp     byte ptr [bx], 0
	jne     scan_next
found_end:
	mov     ax, cx
done:
	pop     di
	pop     si
	pop     bp
	ret
_NextToken ENDP

	END
