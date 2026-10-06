; Serpent Isle SI.EXE, resident segment 85 (file offsets 0x033750 to 0x0337c3, 115 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

	.MODEL  MEDIUM

	PUBLIC  _RandomSeed, _NextRandom, _RandomFromRange, _SetRandomSeed
	PUBLIC  _GetRandomSeed, _GenerateRandomIntegerInRange

	.DATA
_RandomSeed    dd  0BAD0BADh   ; the seed

	.CODE

; seed = seed * 65539 + 1; the new high word is the result
_NextRandom PROC FAR
	mov     cx, word ptr _RandomSeed
	mov     bx, cx
	mov     ax, word ptr _RandomSeed+2
	add     cx, cx
	rcl     ax, 1
	add     cx, bx
	adc     ax, word ptr _RandomSeed+2
	add     ax, bx
	inc     cx
	adc     ax, 0
	mov     word ptr _RandomSeed, cx
	mov     word ptr _RandomSeed+2, ax
	ret
_NextRandom ENDP

; a random number from lo to hi, either way round
_RandomFromRange PROC FAR
	ARG     lo:WORD, hi:WORD
	push    bp
	mov     bp, sp
	push    cs
	call    near ptr _NextRandom
	mov     bx, hi
	mov     cx, lo
	cmp     bx, cx
	jae     ordered
	xchg    bx, cx
ordered:
	xor     dx, dx
	sub     bx, cx
	inc     bx
	jz      whole
	div     bx
whole:
	add     dx, cx
	mov     ax, dx
	pop     bp
	ret
_RandomFromRange ENDP

; the low word is forced odd
_SetRandomSeed PROC FAR
	ARG     seed:DWORD
	push    bp
	mov     bp, sp
	mov     ax, word ptr seed
	or      ax, 1
	mov     word ptr _RandomSeed, ax
	mov     ax, word ptr seed+2
	mov     word ptr _RandomSeed+2, ax
	pop     bp
	ret
_SetRandomSeed ENDP

_GetRandomSeed PROC FAR
	mov     ax, word ptr _RandomSeed
	mov     dx, word ptr _RandomSeed+2
	ret
_GetRandomSeed ENDP

; a random number below n, or 0 when n is 0
_GenerateRandomIntegerInRange PROC FAR
	ARG     n:WORD
	push    bp
	mov     bp, sp
	push    cs
	call    near ptr _NextRandom
	mov     cx, n
	xor     dx, dx
	jcxz    none
	div     cx
none:
	mov     ax, dx
	pop     bp
	ret
_GenerateRandomIntegerInRange ENDP

	END
