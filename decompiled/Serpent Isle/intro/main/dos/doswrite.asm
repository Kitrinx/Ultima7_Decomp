; Serpent Isle INTRO.EXE, resident segment 61 (file offsets 0x0119c0 to 0x011a68, 168 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

	.MODEL  MEDIUM, PASCAL
	LOCALS

	PUBLIC  DOSWRITE
	EXTRN   C MapEmsPointer:FAR         ; maps an EMS pointer in, returns it as a real one

	.DATA
	EXTRN   DosError:WORD               ; DOS error code of the last failure
	EXTRN   DosErrorHandler:DWORD       ; error handler; clearing the code asks for a retry

	.CODE

; Write count bytes from buf, first seeking to pos unless its high word is -1.
; Blocks over 0FFF0h bytes go out in pieces, stepping the buffer's segment.
; Returns 1, or 0 with carry set once the error handler gives up.
DOSWRITE    PROC FAR handle:WORD, pos:DWORD, count:DWORD, buf:DWORD
	LOCAL   cursor:DWORD                ; where the next piece comes from
	USES    si, di, bx, cx, dx
@@retry:
	xor     ax, ax
	cmp     word ptr buf+2, 0
	je      @@error
	push    word ptr buf+2
	push    word ptr buf
	call    MapEmsPointer
	add     sp, 4
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
	mov     ah, 40h
	int     21h
	pop     ds
	jc      @@error
	cmp     ax, cx
	jb      @@full
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
	mov     ah, 40h
	int     21h
	pop     ds
	jc      @@error
	cmp     ax, cx
	jae     @@ok
@@full:
	mov     ax, -1                      ; short write: the disk is full
@@error:
	mov     DosError, ax
	call    DosErrorHandler
	test    DosError, 0FFFFh
	jnz     @@fail
	jmp     @@retry
@@fail:
	xor     ax, ax
	stc
	jmp     short @@exit
@@ok:
	mov     ax, 1
@@exit:
	ret
DOSWRITE    ENDP

	END
