; Black Gate U7.EXE, resident segment 132 (file offsets 0x03e0aa to 0x03e0d5, 43 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.
; Folder inferred from compiler flags and link order.

	.MODEL  MEDIUM, PASCAL
	LOCALS

	PUBLIC  DOSCREATE

	.DATA
	EXTRN   DosError:WORD               ; DOS error code of the last failure
	EXTRN   DosErrorHandler:DWORD       ; error handler; clearing the code asks for a retry

	.CODE

; Create a file, or empty an existing one. Returns the handle, or -1 with carry set.
DOSCREATE   PROC FAR fname:DWORD
	USES    si, di
@@retry:
	push    ds
	lds     dx, fname
	xor     cx, cx
	mov     ah, 3Ch
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
DOSCREATE   ENDP

	END
