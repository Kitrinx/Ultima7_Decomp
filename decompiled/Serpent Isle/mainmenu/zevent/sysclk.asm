; Serpent Isle MAINMENU.EXE, resident segment 37 (file offsets 0x0125ac to 0x01260c, 96 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

	.MODEL  MEDIUM
	.186
	LOCALS

	PUBLIC  _BiosClockVector, _TickChainVector, _BiosClockHandler, _TimerTickHandler
	EXTRN   _TimerDivisor:WORD, _BiosClockCount:WORD, _InterruptsPerTick:WORD
	EXTRN   _InterruptsLeft:WORD, _TickCount:DWORD

	.CODE

_BiosClockVector    dd  DGROUP:0        ; the handlers the two below chain to
_TickChainVector    dd  DGROUP:0

; Timer handler while the chip runs fast: adds the divisor to a running count and passes the
; interrupt to the BIOS clock only when the count wraps, so its time of day keeps its pace.
_BiosClockHandler   PROC FAR
	cli
	push    ax
	push    ds
	mov     ax, DGROUP
	mov     ds, ax
	mov     ax, _TimerDivisor
	or      ax, ax
	jz      @@chain
	add     _BiosClockCount, ax
	jc      @@chain
	mov     al, 20h
	out     20h, al
	pop     ds
	pop     ax
	sti
	iret
@@chain:
	pop     ds
	pop     ax
	jmp     cs:_BiosClockVector
_BiosClockHandler   ENDP

; Timer handler: counts one game tick every so many interrupts (set by SetTimerSpeed), then chains.
_TimerTickHandler   PROC FAR
	pushf
	push    ds
	push    ax
	mov     ax, DGROUP
	mov     ds, ax
	cmp     _InterruptsPerTick, 0
	je      @@pass
	dec     _InterruptsLeft
	jnz     @@pass
	push    es
	pusha
	mov     ax, _InterruptsPerTick
	mov     _InterruptsLeft, ax
	xor     dx, dx
	mov     ax, 1
	add     word ptr _TickCount, ax
	adc     word ptr _TickCount+2, dx
	popa
	pop     es
@@pass:
	pop     ax
	pop     ds
	popf
	jmp     cs:_TickChainVector
_TimerTickHandler   ENDP

	END
