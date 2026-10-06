; Black Gate U7.EXE, resident segment 67 (file offsets 0x026976 to 0x0269f0, 122 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.
; Folder inferred from compiler flags and link order.

	.MODEL  MEDIUM
	.386
	LOCALS

	PUBLIC  _SoundIrqHandler
	EXTRN   _DmaDoneHandler:DWORD

	.DATA
stackOverrun    dw  0                   ; set when the handler ran into the stack guard
savedSp         dw  0
savedSs         dw  0
stackGuard      dw  0FFFFh
				db  512 dup (0)         ; the handler's own stack
				dw  0FFFFh

	.CODE

savedAx dd  0

; Sound card interrupt: on its own stack, calls DmaDoneHandler when one is set.
_SoundIrqHandler    PROC FAR
	mov     word ptr cs:savedAx, ax
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
	add     ax, 512
	mov     sp, ax
	mov     ax, word ptr cs:savedAx
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
	mov     eax, _DmaDoneHandler
	or      eax, eax
	je      @@noHandler
	call    _DmaDoneHandler
@@noHandler:
	mov     ax, stackGuard
	cmp     ax, 0FFFFh
	je      @@stackOk
	mov     stackOverrun, 1
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
	popf
	iret
_SoundIrqHandler    ENDP

	END
