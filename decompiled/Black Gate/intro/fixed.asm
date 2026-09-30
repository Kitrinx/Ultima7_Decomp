; Black Gate INTRO.EXE, one module of resident segment 1 (file offsets 0x009238 to 0x009399, 353 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

; 16.16 fixed-point arithmetic for the 386: a long whose high word is the
; integer part and whose low word is the fraction. Callable from C as
;
;   long far FixedDistance(long dx, long dy);
;   long far IntToFixed(int n);
;   int far FixedToInt(long f);
;   long far FixedDiv(long a, long b);
;   long far FixedMul(long a, long b);
;
; Results come back in DX:AX.

	.386

	PUBLIC  _FixedDistance, _IntToFixed, _FixedToInt, _FixedDiv, _FixedMul

FIXED_TEXT SEGMENT DWORD PUBLIC USE16 'CODE'
	ASSUME  cs:FIXED_TEXT

; Roughly the length of (dx, dy): the longer side plus 11/32 of the shorter.
_FixedDistance  PROC FAR
	enter   0, 0
	mov     eax, [bp+6]
	mov     ebx, [bp+10]
	or      eax, eax
	jns     @@xpos
	neg     eax
@@xpos:
	or      ebx, ebx
	jns     @@ypos
	neg     ebx
@@ypos:
	cmp     eax, ebx
	jae     @@ordered
	xchg    eax, ebx
@@ordered:
	shr     ebx, 2                  ; shorter / 4
	push    ebx
	shr     ebx, 1                  ; shorter / 8
	push    ebx
	shr     ebx, 2                  ; less shorter / 32
	neg     ebx
	add     eax, ebx
	pop     ebx
	add     eax, ebx
	pop     ebx
	add     eax, ebx
	push    eax                     ; EAX into DX:AX
	pop     ax
	pop     dx
	leave
	ret
_FixedDistance  ENDP

; n, as 16.16.
_IntToFixed     PROC FAR
	enter   0, 0
	mov     dx, [bp+6]
	xor     ax, ax
	leave
	ret
_IntToFixed     ENDP

; The integer part of a 16.16 value.
_FixedToInt     PROC FAR
	enter   0, 0
	mov     ax, [bp+8]
	leave
	ret
_FixedToInt     ENDP

; a / b by shift and subtract: the whole quotient first, then 16 fraction bits.
_FixedDiv       PROC FAR
	enter   0, 0
	push    esi
	push    edi
	xor     cx, cx                  ; counts the negative operands
	mov     esi, [bp+10]
	or      esi, esi
	jns     @@divisor
	inc     cx
	neg     esi
@@divisor:
	mov     eax, [bp+6]
	or      eax, eax
	jns     @@dividend
	inc     cx
	neg     eax
@@dividend:
	push    cx
	mov     ch, 32                  ; dividend bits
	xor     ebx, ebx                ; remainder
@@skip:                                 ; leading zero bits leave the quotient empty
	clc
	rcl     eax, 1
	jb      @@first
	dec     ch
	jnz     @@skip
	jmp     short @@sign
@@next:
	clc
	rcl     eax, 1
@@first:
	rcl     ebx, 1
	sub     ebx, esi
	jb      @@restore
	inc     ax
	dec     ch
	jnz     @@next
	jmp     short @@fraction
@@restore:
	add     ebx, esi
	dec     ch
	jnz     @@next
@@fraction:
	mov     cx, 16                  ; fraction bits
@@bit:
	shl     eax, 1
	shl     ebx, 1
	sub     ebx, esi
	jb      @@undo
	inc     ax
	loop    @@bit
	jmp     short @@sign
@@undo:
	add     ebx, esi
	loop    @@bit
@@sign:
	pop     cx
	dec     cx                      ; one negative operand negates the result
	jnz     @@done
	neg     eax
@@done:
	push    eax
	pop     ax
	pop     dx
	pop     edi
	pop     esi
	leave
	ret
_FixedDiv       ENDP

; a * b: the 64-bit product of the magnitudes, shifted down 16 bits.
_FixedMul       PROC FAR
	enter   0, 0
	push    esi
	push    edi
	xor     ax, ax                  ; counts the negative operands
	mov     cx, [bp+6]
	mov     si, [bp+8]
	or      si, si
	jns     @@apos
	inc     ax
	neg     cx                      ; negate SI:CX
	adc     si, 0
	neg     si
@@apos:
	mov     bx, [bp+10]
	mov     di, [bp+12]
	or      di, di
	jns     @@bpos
	inc     ax
	neg     bx                      ; negate DI:BX
	adc     di, 0
	neg     di
@@bpos:
	push    ax
	mov     ax, bx
	mul     cx
	push    ax
	xchg    dx, cx
	mov     ax, di
	mul     dx
	add     cx, ax
	adc     dx, 0
	xchg    bx, dx
	mov     ax, si
	mul     dx
	add     cx, ax
	adc     bx, dx
	mov     ax, di
	mul     si
	xchg    cx, dx
	add     bx, ax
	adc     cx, 0
	pop     ax
	mov     si, 16                  ; CX:BX:DX:AX >> 16
@@shift:
	shr     cx, 1
	rcr     bx, 1
	rcr     dx, 1
	rcr     ax, 1
	dec     si
	jnz     @@shift
	pop     di
	dec     di
	jnz     @@positive
	neg     ax
	adc     dx, 0
	neg     dx
@@positive:
	pop     edi
	pop     esi
	leave
	ret
_FixedMul       ENDP

FIXED_TEXT ENDS
	END
