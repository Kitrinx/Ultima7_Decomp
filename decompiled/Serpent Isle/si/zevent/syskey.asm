; Serpent Isle SI.EXE, resident segment 31 (file offsets 0x019922 to 0x019967, 69 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.
; Folder chosen by subsystem.

	.MODEL  MEDIUM
	LOCALS

	PUBLIC  _PrevKeyIntercept, _InterceptCtrlC

SHIFT_FLAGS EQU 17h                     ; in the BIOS data area; bit 2 is Ctrl
SCAN_C      EQU 2Eh

	.CODE

_PrevKeyIntercept   dd  DGROUP:0        ; the handler this one chains to
extended            db  0               ; the last scan code was the E0 prefix

; Keyboard intercept (INT 15h, AH=4Fh), called by the BIOS with DS at its data area: swallows
; Ctrl+C and lets every other key through.
_InterceptCtrlC PROC FAR
	pushf
	cmp     ah, 4Fh
	je      @@intercept
	popf
	jmp     cs:_PrevKeyIntercept
@@intercept:
	cmp     al, 0E0h
	jne     @@notPrefix
	mov     cs:extended, al
	jmp     short @@pass
@@notPrefix:
	test    cs:extended, 0E0h
	mov     cs:extended, 0
	jnz     @@pass
	cmp     al, SCAN_C
	jne     @@pass
	test    word ptr ds:[SHIFT_FLAGS], 4
	jnz     @@ctrlC
@@pass:
	popf
	stc
	jmp     cs:_PrevKeyIntercept
@@ctrlC:
	mov     al, 15h
	popf
	clc
	jmp     cs:_PrevKeyIntercept
_InterceptCtrlC ENDP

	END
