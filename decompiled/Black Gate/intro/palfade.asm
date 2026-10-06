; Black Gate MAINMENU.EXE, one module of resident segment 19 (file offsets 0x00ecda to 0x00ed72, 152 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

	.MODEL  MEDIUM, C
	LOCALS

	PUBLIC  PrepareFade, StepFade
	PUBLIC  FadeSize, FadeStepsLeft

	.DATA

FadeSize        dw      0               ; components being faded
FadeStepsLeft   dw      0

	.CODE

; Fill deltas with each component's step from current to target over steps,
; and remember size and steps for StepFade.
PrepareFade         PROC FAR current:WORD, target:WORD, size:WORD, steps:WORD, deltas:WORD
	push    es
	push    si
	push    di
	mov     cx, size
	mov     FadeSize, cx
	push    ds
	pop     es
	mov     si, current
	add     si, cx
	mov     di, target
	add     di, cx
	mov     bx, deltas
	add     bx, cx
	add     bx, cx                      ; one word per component
@@next:
	or      cx, cx
	je      @@done
	dec     cx
	dec     bx
	dec     bx
	dec     si
	dec     di
	xor     ax, ax
	mov     ah, [di]
	xor     dx, dx
	mov     dh, [si]
	sub     ax, dx
	cwd
	idiv    steps
	mov     [bx], ax
	jmp     @@next
@@done:
	mov     ax, steps
	mov     FadeStepsLeft, ax
	pop     di
	pop     si
	pop     es
	ret
PrepareFade         ENDP

; Add each component's delta to its color and fraction bytes.
; Returns 1 once no steps are left, else 0.
StepFade            PROC FAR colors:WORD, deltas:WORD, fractions:WORD
	push    es
	push    di
	push    si
	mov     ax, FadeStepsLeft
	or      ax, ax
	je      @@finished
	dec     ax
	mov     FadeStepsLeft, ax
	mov     cx, FadeSize
	push    ds
	pop     es
	mov     di, colors
	add     di, cx
	mov     si, deltas
	add     si, cx
	add     si, cx                      ; one word per component
	mov     bx, fractions
	add     bx, cx
@@next:
	or      cx, cx
	je      @@stepped
	dec     cx
	dec     si
	dec     si
	dec     di
	dec     bx
	mov     dx, [si]
	or      dx, dx
	je      @@next
	xor     ax, ax
	mov     ah, [di]
	mov     al, [bx]
	add     ax, dx
	mov     [di], ah
	mov     [bx], al
	jmp     @@next
@@finished:
	mov     ax, 1
	jmp     @@exit
@@stepped:
	xor     ax, ax
@@exit:
	pop     si
	pop     di
	pop     es
	ret
StepFade            ENDP

	END
