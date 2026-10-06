; Serpent Isle INTRO.EXE, resident segment 75 (file offsets 0x014c38 to 0x014c8a, 82 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

.MODEL  MEDIUM
	.386
	LOCALS

	PUBLIC  _SetA20Gate

	.CODE

; Waits for the keyboard controller's input buffer to empty; ZF set once it has.
WaitForKeyboardController PROC FAR
	xor     cx, cx
@@waitEmpty:
	in      al, 64h
	and     al, 2
	loopnz  @@waitEmpty
	ret
WaitForKeyboardController ENDP

; Opens (on nonzero) or closes the A20 line through the keyboard controller's output port.
; Returns 1 on success, 0 if the controller never answered.
_SetA20Gate PROC FAR
	ARG     enable:WORD
	enter   0, 0
	mov     ax, enable
	or      ax, ax
	mov     ah, 0DFh                    ; output port value with A20 on
	jnz     @@send
	mov     ah, 0DDh                    ; and off
@@send:
	push    cs
	call    near ptr WaitForKeyboardController
	jnz     @@failed
	mov     al, 0D1h                    ; write the output port
	out     64h, al
	push    cs
	call    near ptr WaitForKeyboardController
	jnz     @@failed
	mov     al, ah
	out     60h, al
	push    cs
	call    near ptr WaitForKeyboardController
	jnz     @@failed
	xor     cx, cx
	mov     al, 0FFh                    ; pulse no output lines: a null command
	out     64h, al
	push    cs
	call    near ptr WaitForKeyboardController
	jnz     @@failed
	mov     ax, 1
	jmp     @@done
@@failed:
	xor     ax, ax
@@done:
	leave
	ret
_SetA20Gate ENDP

	END
