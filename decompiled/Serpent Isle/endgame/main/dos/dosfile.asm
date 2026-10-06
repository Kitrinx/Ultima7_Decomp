; Serpent Isle ENDGAME.EXE, resident segment 58 (file offsets 0x010ca0 to 0x010cec, 76 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

	.MODEL  MEDIUM, PASCAL
	LOCALS

	PUBLIC  DosError, DosErrorHandler, DOSOPEN, AcceptDosError, DOSCLOSE

	.DATA
DosError        dw  0                   ; DOS error code of the last failure
DosErrorHandler dd  AcceptDosError      ; error handler; clearing the code asks for a retry

	.CODE

; Open a file for reading and writing. Returns the handle, or -1 with carry set.
DOSOPEN PROC FAR fname:DWORD
	mov     DosError, 0
@@retry:
	push    ds
	lds     dx, fname
	mov     ax, 3D02h
	int     21h
	pop     ds
	jnc     @@done
	mov     DosError, ax
	call    DosErrorHandler
	test    DosError, 0FFFFh
	jz      @@retry
	mov     ax, -1
	stc
@@done:
	ret
DOSOPEN ENDP

; The default error handler: leaves the code set, so the failing call gives up.
AcceptDosError  PROC FAR
	ret
AcceptDosError  ENDP

; Close a file.
DOSCLOSE    PROC FAR handle:WORD
@@retry:
	mov     bx, handle
	mov     ah, 3Eh
	int     21h
	jnc     @@done
	mov     DosError, ax
	call    DosErrorHandler
	test    DosError, 0FFFFh
	jz      @@retry
@@done:
	ret
DOSCLOSE    ENDP

	END
