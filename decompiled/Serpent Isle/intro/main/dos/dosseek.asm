; Serpent Isle INTRO.EXE, resident segment 60 (file offsets 0x011988 to 0x0119c0, 56 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

	.MODEL  MEDIUM, PASCAL
	LOCALS

	PUBLIC  DOSSEEK

	.DATA
	EXTRN   DosError:WORD               ; DOS error code of the last failure
	EXTRN   DosErrorHandler:DWORD       ; error handler; clearing the code asks for a retry

	.CODE

; Move the file pointer to pos from where method says (0 start, 1 current, 2 end).
; Returns the new position in DX:AX, or -1 with carry set.
DOSSEEK    PROC FAR handle:WORD, pos:DWORD, method:WORD
	USES    si, di
	mov     DosError, 0
@@retry:
	mov     bx, handle
	mov     ax, method
	mov     dx, word ptr pos
	mov     cx, word ptr pos+2
	mov     ah, 42h
	int     21h
	jnc     @@done
	mov     DosError, ax
	call    DosErrorHandler
	test    DosError, 0FFFFh
	jz      @@retry
	mov     ax, -1
	mov     dx, ax
	stc
@@done:
	ret
DOSSEEK    ENDP

	END
