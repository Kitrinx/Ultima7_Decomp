; Serpent Isle ENDGAME.EXE, resident segment 25 (file offsets 0x00c1a8 to 0x00c213, 107 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

	.MODEL  MEDIUM, PASCAL
	LOCALS

DAC_READ        EQU     3C7h            ; DAC read index
DAC_DATA        EQU     3C9h

	PUBLIC  READDACENTRY, READDAC

; Give the DAC time between port accesses.
IODELAY MACRO
	LOCAL   next
	jmp     next
next:
ENDM

	.CODE

; Read one DAC color into three bytes: red, green, blue. A null color reads nothing.
READDACENTRY    PROC FAR index:WORD, color:WORD
	USES    di
	push    ds
	pop     es
	cld
	mov     dx, DAC_READ
	mov     di, color
	or      di, di
	je      @@done
	mov     ax, index
	out     dx, al
	IODELAY
	mov     dx, DAC_DATA
	xor     ah, ah
	in      al, dx
	IODELAY
	stosb
	in      al, dx
	IODELAY
	stosb
	in      al, dx
	IODELAY
	stosb
@@done:
	ret
READDACENTRY    ENDP

; Read count DAC colors, three bytes each, from first on.
READDAC         PROC FAR first:WORD, count:WORD, colors:WORD
	USES    di
	push    ds
	pop     es
	mov     di, colors
	or      di, di
	je      @@done
	mov     cx, count
	mov     bx, first
	cld
@@color:
	mov     dx, DAC_READ
	mov     al, bl
	out     dx, al
	IODELAY
	mov     dx, DAC_DATA
	xor     ah, ah
	in      al, dx
	IODELAY
	stosb
	in      al, dx
	IODELAY
	stosb
	in      al, dx
	IODELAY
	stosb
	inc     bl
	loop    @@color
@@done:
	ret
READDAC         ENDP

	END
