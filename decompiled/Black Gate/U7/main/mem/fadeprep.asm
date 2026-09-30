; Black Gate U7.EXE, one module of resident segment 27 (file offsets 0x0151f0 to 0x015309, 281 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.
; Segment 27 joins 25 modules' code as LOWLEVEL_TEXT, in link order and doubleword aligned.
; This one starts at 049Ch; its own empty segment 147 fixes its link position.
; Holds palette format conversion and fade set-up.

	.MODEL  MEDIUM
	.386
	LOCALS

PALETTE_SIZE    EQU 768                 ; 256 colors of three components

	PUBLIC  _PaletteWordsToBytes, _PaletteBytesToWords, _PreparePaletteFade

LOWLEVEL_TEXT   SEGMENT DWORD PUBLIC USE16 'CODE'
	ASSUME  cs:LOWLEVEL_TEXT

; Copy 768 palette words to bytes, keeping each high byte.
_PaletteWordsToBytes    PROC FAR
	ARG     dest:DWORD, src:DWORD
	enter   0, 0
	push    edi
	push    esi
	push    ds
	push    es
	push    ds
	pop     es
	mov     edi, dest
	mov     esi, src
	mov     cx, PALETTE_SIZE
	xor     eax, eax
	mov     es, ax
	mov     ds, ax
@@entry:
	xor     eax, eax
	mov     ax, [esi]
	mov     al, ah
	xor     ah, ah
	mov     [edi], al
	inc     edi
	inc     esi
	inc     esi
	dec     cx
	jne     @@entry
	pop     es
	pop     ds
	pop     esi
	pop     edi
	leave
	ret
_PaletteWordsToBytes    ENDP

; Copy 768 palette bytes to words, each byte becoming the high byte.
_PaletteBytesToWords    PROC FAR
	ARG     dest:DWORD, src:DWORD
	enter   0, 0
	push    edi
	push    esi
	push    ds
	push    es
	push    ds
	pop     es
	mov     edi, dest
	mov     esi, src
	mov     cx, PALETTE_SIZE
	xor     eax, eax
	mov     es, ax
	mov     ds, ax
@@entry:
	xor     eax, eax
	mov     al, [esi]
	mov     ah, al
	xor     al, al
	mov     [edi], ax
	inc     edi
	inc     edi
	inc     esi
	dec     cx
	jne     @@entry
	pop     es
	pop     ds
	pop     esi
	pop     edi
	leave
	ret
_PaletteBytesToWords    ENDP

; Set up a fade of 768 components: store each gap between the current
; words and the target bytes, and that gap over steps rounded away from zero.
; Returns steps.
_PreparePaletteFade PROC FAR
	ARG     current:DWORD, target:DWORD, gaps:DWORD, deltas:DWORD, steps:WORD
	enter   0, 0
	push    edi
	push    esi
	push    ds
	push    es
	push    ds
	pop     es
	mov     cx, PALETTE_SIZE
	mov     esi, current
	mov     edi, target
	xor     eax, eax
	mov     ds, ax
@@entry:
	xor     ax, ax
	xor     bx, bx
	mov     ax, [esi]
	inc     esi
	inc     esi
	mov     bl, [edi]
	mov     bh, bl
	xor     bl, bl
	inc     edi
	push    edi
	mov     edi, gaps
	sub     ax, bx
	mov     [edi], ax
	inc     edi
	inc     edi
	mov     gaps, edi
	mov     edi, deltas
	or      ax, ax
	jne     @@divide
	mov     dx, 0
	mov     ax, 0
	jmp     @@round
@@divide:
	push    cx
	xor     edx, edx
	cwd
	mov     cx, steps
	idiv    cx
	pop     cx
; round any remainder away from zero
@@round:
	or      dx, dx
	je      @@store
	cmp     ax, 0
	jl      @@down
	jg      @@up
	cmp     dx, 0
	jl      @@downSmall
	inc     ax
	jmp     @@store
@@downSmall:
	dec     ax
	jmp     @@store
@@up:
	inc     ax
	jmp     @@store
@@down:
	dec     ax
@@store:
	mov     [edi], ax
	inc     edi
	inc     edi
	mov     deltas, edi
	pop     edi
	dec     cx
	jne     @@entry
	mov     ax, steps
	pop     es
	pop     ds
	pop     esi
	pop     edi
	leave
	ret
_PreparePaletteFade ENDP

LOWLEVEL_TEXT   ENDS

	END
