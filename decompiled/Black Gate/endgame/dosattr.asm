; Black Gate ENDGAME.EXE, resident segment 66 (file offsets 0x01175c to 0x01179a, 62 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

	.MODEL  MEDIUM, C
	LOCALS

DOS_ATTRIBUTES  EQU     43h             ; get or set a file's attributes
GET_ATTRIBUTES  EQU     0
ATTR_ERROR      EQU     8000h           ; marks a DOS error code in the result

	PUBLIC  DosGetAttributes, DosFileExists

	.CODE

; A file's attributes, or the DOS error code with the top bit set.
DosGetAttributes    PROC FAR fileName:DWORD
	push    ds
	mov     ax, word ptr fileName+2
	mov     ds, ax
	mov     dx, word ptr fileName
	mov     ah, DOS_ATTRIBUTES
	mov     al, GET_ATTRIBUTES
	int     21h
	pop     ds
	jnc     @@found
	or      ax, ATTR_ERROR
	jmp     @@done
@@found:
	mov     ax, cx
@@done:
	ret
DosGetAttributes    ENDP

; 1 when the file is there, else 0.
DosFileExists       PROC FAR fileName:DWORD
	push    ds
	mov     ax, word ptr fileName+2
	mov     ds, ax
	mov     dx, word ptr fileName
	mov     ah, DOS_ATTRIBUTES
	mov     al, GET_ATTRIBUTES
	int     21h
	pop     ds
	jnc     @@found
	xor     ax, ax
	jmp     @@done
@@found:
	mov     ax, 1
@@done:
	ret
DosFileExists       ENDP

	END
