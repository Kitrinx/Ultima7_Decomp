; Serpent Isle SI.EXE, resident segment 129 (file offsets 0x03d9be to 0x03dabb, 253 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

	.MODEL  MEDIUM, PASCAL
	LOCALS
	JUMPS

	PUBLIC  DosBytesRead, DOSREAD

	.DATA
	EXTRN   DosError:WORD               ; DOS error code of the last failure
	EXTRN   DosErrorHandler:DWORD       ; error handler; clearing the code asks for a retry
DosBytesRead    dd  0                   ; bytes read so far by the current call

	.CODE

; Read up to count bytes into buf, first seeking to pos unless its high word is -1.
; Blocks over 0FFF0h bytes come in pieces, stepping the buffer's segment.
; Returns the bytes read in DX:AX, or 0 with carry set once the error handler gives up.
DOSREAD    PROC FAR handle:WORD, pos:DWORD, count:DWORD, buf:DWORD
	LOCAL   cursor:DWORD                ; where the next piece goes
	USES    si, di, bx, cx
	mov     DosError, 0
	mov     ax, word ptr count
	or      ax, word ptr count+2
	cmp     ax, 0
	jne     @@retry
	clc
	xor     dx, dx
	jmp     @@exit
@@retry:
	xor     ax, ax
	mov     word ptr DosBytesRead+2, ax
	mov     word ptr DosBytesRead, ax
	inc     ax
	cmp     word ptr buf+2, 0
	jne     @@start
	cmp     word ptr buf, 0
	jne     @@start
	jmp     @@error
@@start:
	mov     dx, word ptr buf+2
	mov     ax, word ptr buf
	mov     word ptr cursor+2, dx
	mov     word ptr cursor, ax
	mov     bx, handle
	mov     dx, word ptr pos
	mov     cx, word ptr pos+2
	cmp     cx, -1
	je      @@noSeek
	mov     ax, 4200h
	int     21h
	jc      @@error
@@noSeek:
	mov     di, word ptr count+2
	mov     si, word ptr count
@@next:
	or      di, di
	jnz     @@piece
	cmp     si, 0FFF0h
	jbe     @@last
@@piece:
	mov     cx, 0FFF0h
	push    ds
	lds     dx, cursor
	mov     ah, 3Fh
	int     21h
	pop     ds
	jc      @@error
	add     word ptr DosBytesRead, ax
	adc     word ptr DosBytesRead+2, 0
	cmp     ax, 0
	je      @@done                      ; end of file
	sub     si, ax
	sbb     di, 0
	shr     ax, 1
	shr     ax, 1
	shr     ax, 1
	shr     ax, 1
	add     word ptr cursor+2, ax
	jmp     @@next
@@last:
	mov     cx, si
	push    ds
	lds     dx, cursor
	mov     ah, 3Fh
	int     21h
	pop     ds
	jnc     @@done
@@error:
	or      ax, ax
	jnz     @@report
	inc     ax
@@report:
	mov     DosError, ax
	call    DosErrorHandler
	test    DosError, 0FFFFh
	jz      @@retry
	xor     ax, ax
	xor     dx, dx
	stc
	jmp     short @@exit
@@done:
	add     word ptr DosBytesRead, ax
	adc     word ptr DosBytesRead+2, 0
	mov     ax, word ptr DosBytesRead
	mov     dx, word ptr DosBytesRead+2
@@exit:
	ret
DOSREAD    ENDP

	END
