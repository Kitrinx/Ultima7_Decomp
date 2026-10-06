; Serpent Isle INTRO.EXE, resident segment 25 (file offsets 0x00cdee to 0x00ce44, 86 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

.MODEL  MEDIUM, PASCAL
	LOCALS

DAC_WRITE       EQU     3C8h            ; DAC write index; the data port follows it

	PUBLIC  WRITEDACENTRY, WRITEDAC

; Give the DAC time between port accesses.
IODELAY MACRO
	LOCAL   next
	jmp     next
next:
ENDM

	.CODE

; Load one DAC color from three bytes: red, green, blue.
WRITEDACENTRY   PROC FAR index:WORD, color:WORD
	USES    si
	mov     dx, DAC_WRITE
	mov     si, color
	mov     ax, index
	out     dx, al
	IODELAY
	inc     dx
	lodsb
	out     dx, al
	IODELAY
	lodsb
	out     dx, al
	IODELAY
	lodsb
	out     dx, al
	IODELAY
	ret
WRITEDACENTRY   ENDP

; Load count DAC colors, three bytes each, from first on.
WRITEDAC        PROC FAR first:WORD, count:WORD, colors:WORD
	USES    si
	mov     dx, DAC_WRITE
	mov     si, colors
	mov     cx, count
	mov     bx, first
@@color:
	mov     al, bl
	out     dx, al
	IODELAY
	inc     dx
	lodsb
	out     dx, al
	IODELAY
	lodsb
	out     dx, al
	IODELAY
	lodsb
	out     dx, al
	IODELAY
	dec     dx
	inc     bl
	loop    @@color
	ret
WRITEDAC        ENDP

	END
