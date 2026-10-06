; Serpent Isle SI.EXE, resident segment 111 (file offsets 0x03b3a0 to 0x03b466, 198 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

	.MODEL  MEDIUM
	.386
	LOCALS

	PUBLIC  _DispatchMouseEvents, _MouseStackOverrun
	EXTRN   _MouseHandlerCount:WORD, _MouseX:WORD, _MouseY:WORD
	EXTRN   _MouseHandlers:DWORD, _MouseHandlerMasks:WORD

	.DATA
_MouseStackOverrun  dw  0               ; set when a handler ran into the stack guard
savedSp             dw  0
savedSs             dw  0
stackGuard          dw  0FFFFh
					db  1024 dup (0)    ; the handlers' own stack
					dw  0FFFFh

	.CODE

busy    db  0                           ; nonzero while the handlers run
events  dw  0                           ; the driver's event mask
slot    dw  0

; Mouse driver callback (INT 33h, AX=14h): on its own stack, calls every registered handler
; whose mask matches the events, last registered first, with (events, bx, cx, dx).
_DispatchMouseEvents    PROC FAR
	cmp     cs:busy, 0
	je      @@enter
	ret
@@enter:
	mov     cs:busy, 1
	mov     cs:events, ax
	pushf
	push    ds
	mov     ax, DGROUP
	mov     ds, ax
	cli
	mov     savedSs, ss
	mov     savedSp, sp
	mov     ss, ax
	mov     ax, offset stackGuard
	inc     ax
	inc     ax
	and     ax, 0FFFEh
	add     ax, 1024
	mov     sp, ax
	mov     ax, cs:events
	push    eax
	push    ebx
	push    ecx
	push    edx
	push    edi
	push    esi
	push    ds
	push    es
	push    fs
	push    gs
	sti
	push    dx
	push    cx
	push    bx
	mov     ax, cs:events
	push    ax
	mov     bx, ax
	mov     _MouseX, cx
	mov     _MouseY, dx
	mov     si, _MouseHandlerCount
	dec     si
	js      @@allCalled
	shl     si, 1
@@nextHandler:
	mov     ax, cs:events
	and     ax, _MouseHandlerMasks[si]
	jz      @@skip
	shl     si, 1
	mov     cs:slot, si
	call    _MouseHandlers[si]
	mov     si, cs:slot
	shr     si, 1
@@skip:
	dec     si
	dec     si
	jns     @@nextHandler
@@allCalled:
	add     sp, 8
	mov     ax, stackGuard
	cmp     ax, 0FFFFh
	je      @@stackOk
	mov     _MouseStackOverrun, 1
@@stackOk:
	cli
	pop     gs
	pop     fs
	pop     es
	pop     ds
	pop     esi
	pop     edi
	pop     edx
	pop     ecx
	pop     ebx
	pop     eax
	mov     ss, savedSs
	mov     sp, savedSp
	pop     ds
	mov     cs:busy, 0
	popf
	ret
_DispatchMouseEvents    ENDP

	END
