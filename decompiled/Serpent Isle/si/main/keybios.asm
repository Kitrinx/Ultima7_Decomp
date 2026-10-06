; Serpent Isle SI.EXE, resident segment 17 (file offsets 0x012654 to 0x0126b3, 95 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

	.MODEL  MEDIUM, C

	PUBLIC  ReadBiosKey

	.CODE

; The next key from the BIOS, or 0 when none is waiting. A key with an
; ASCII code comes back as that code; a key without one, and backspace and
; the keypad keys other than grey + and -, as its scan code with AH = 1.
; With latest set, the keys waiting are all read and the last one is kept.
ReadBiosKey PROC FAR USES ds si di, latest:BYTE
	test    latest, 0FFh
	je      peek
	xor     ax, ax
drain:
	push    ax
	mov     ah, 1
	int     16h
	pop     ax
	je      got
	mov     ah, 0
	int     16h
	jmp     drain
peek:
	mov     ah, 1
	int     16h
	mov     ax, 0
	je      got
	mov     ah, 0
	int     16h
got:
	or      al, al
	jne     ascii
	or      ah, ah
	je      done
	mov     al, ah
	mov     ah, 1
	jmp     short done
ascii:
	cmp     ah, 47h
	jb      notpad
	cmp     ah, 51h
	ja      notpad
	cmp     ah, 4Ah
	je      plain
	cmp     ah, 4Eh
	je      plain
	jmp     short scan
notpad:
	cmp     ah, 0Eh
	jne     plain
scan:
	mov     al, ah
	mov     ah, 1
	jmp     short done
plain:
	xor     ah, ah
done:
	ret
ReadBiosKey ENDP

	END
