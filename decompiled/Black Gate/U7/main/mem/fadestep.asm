; Black Gate U7.EXE, one module of resident segment 27 (file offsets 0x01530c to 0x0153a6, 154 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.
; Segment 27 joins 25 modules' code as LOWLEVEL_TEXT, in link order and doubleword aligned.
; This one starts at 05B8h; its own empty segment 148 fixes its link position.
; Holds one step of a palette fade.

	.MODEL  MEDIUM
	.386
	LOCALS

PALETTE_SIZE    EQU 768                 ; 256 colors of three components

	PUBLIC  _StepPaletteFade

LOWLEVEL_TEXT   SEGMENT DWORD PUBLIC USE16 'CODE'
	ASSUME  cs:LOWLEVEL_TEXT

; Move 768 fading components one step, never past the remaining gap;
; a component whose gap runs out stops.
_StepPaletteFade    PROC FAR
	ARG     colors:DWORD, gaps:DWORD, deltas:DWORD
	enter   0, 0
	push    edi
	push    esi
	push    ds
	push    es
	push    ds
	pop     es
	mov     cx, PALETTE_SIZE
	mov     esi, gaps
	mov     edi, colors
	mov     ebx, deltas
	xor     eax, eax
	mov     ds, ax
@@entry:
	xor     eax, eax
	mov     ax, [edi]
	push    cx
	xor     dx, dx
	xor     cx, cx
	mov     dx, [esi]
	mov     cx, [ebx]
	or      cx, cx
	je      @@idle
	cmp     dx, 0
	jl      @@negative
	cmp     cx, dx
	jae     @@lastUp
	sub     ax, cx
	sub     dx, cx
	jmp     @@store
@@lastUp:
	sub     ax, dx
	xor     dx, dx
	mov     word ptr [ebx], 0
	jmp     @@store
; negative gap: step while the delta still fits
@@negative:
	cmp     cx, dx
	jl      @@lastDown
	sub     ax, cx
	sub     dx, cx
	jmp     @@store
@@lastDown:
	sub     ax, dx
	xor     dx, dx
	mov     word ptr [ebx], 0
@@store:
	mov     [esi], dx
	inc     esi
	inc     esi
	inc     ebx
	inc     ebx
	mov     [edi], ax
	inc     edi
	inc     edi
	jmp     @@next
@@idle:
	inc     esi
	inc     esi
	inc     ebx
	inc     ebx
	inc     edi
	inc     edi
@@next:
	pop     cx
	dec     cx
	jne     @@entry
	pop     es
	pop     ds
	pop     esi
	pop     edi
	leave
	ret
_StepPaletteFade    ENDP

LOWLEVEL_TEXT   ENDS

	END
