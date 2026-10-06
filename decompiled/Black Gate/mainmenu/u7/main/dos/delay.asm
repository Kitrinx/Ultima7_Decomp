; Black Gate U7.EXE, resident segment 123 (file offsets 0x03da4e to 0x03db6c, 286 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.
; Folder inferred from compiler flags and link order.

	.MODEL  MEDIUM
	LOCALS

	PUBLIC  _SpinActive, _TickSampleCount, _TickSamples, _SpinCounter
	PUBLIC  _OldTimerHandler, _SpinsPerTick, _CalibrationHandler, _DelayCalibrated
	PUBLIC  _SpinCount, _SampleCalibrationTick, _CalibrateDelay, _WaitTicks

	.DATA
_SpinActive         db  0                       ; nonzero while counting down
_TickSampleCount    db  0                       ; samples taken
_TickSamples        dd  4 dup (0)               ; the counter at four timer ticks
_SpinCounter        dd  0                       ; spins left
_OldTimerHandler    dd  0                       ; the previous timer handler
_SpinsPerTick       dw  1                       ; counts per tick, over 256
_CalibrationHandler dd  _SampleCalibrationTick  ; the sampling handler below
_DelayCalibrated    db  0                       ; nonzero once calibrated

	.CODE

; Count spins down to zero.
_SpinCount  PROC FAR
	ARG     spins:DWORD
	push    bp
	mov     bp, sp
	mov     ax, word ptr spins+2
	mov     word ptr _SpinCounter+2, ax
	mov     cx, word ptr spins
	mov     word ptr _SpinCounter, cx
	mov     _TickSampleCount, 0
	inc     _SpinActive
	jcxz    @@nextHigh
@@count:
	dec     word ptr _SpinCounter
	jnz     @@count
@@nextHigh:
	cmp     word ptr _SpinCounter+2, 0
	je      @@finished
	dec     word ptr _SpinCounter+2
	jmp     @@count
@@finished:
	dec     _SpinActive
	pop     bp
	ret
_SpinCount  ENDP

; Timer handler while calibrating: records the counter on each of four ticks,
; then cuts the count short. Always chains to the previous handler.
_SampleCalibrationTick  PROC FAR
	push    ax
	push    bx
	push    cx
	push    dx
	push    es
	push    ds
	push    si
	push    di
	push    bp
	mov     bp, sp
	mov     ax, @data
	mov     ds, ax
	cmp     _SpinActive, 0
	je      @@chain
	mov     bl, _TickSampleCount
	xor     bh, bh
	cmp     bl, 4
	jb      @@sample
	mov     word ptr _SpinCounter, 1
	mov     word ptr _SpinCounter+2, 1
	mov     _SpinActive, 0
	jmp     short @@chain
@@sample:
	shl     bx, 1
	shl     bx, 1
	mov     ax, word ptr _SpinCounter
	mov     word ptr _TickSamples[bx], ax
	mov     ax, word ptr _SpinCounter+2
	mov     word ptr _TickSamples[bx+2], ax
	inc     _TickSampleCount
@@chain:
	pushf
	call    _OldTimerHandler
	mov     sp, bp
	pop     bp
	pop     di
	pop     si
	pop     ds
	pop     es
	pop     dx
	pop     cx
	pop     bx
	pop     ax
	iret
_SampleCalibrationTick  ENDP

; Time the count between timer ticks, once.
_CalibrateDelay PROC FAR
	push    si
	push    di
	push    ds
	cmp     _DelayCalibrated, 0
	jne     @@calibrated
	mov     _DelayCalibrated, 1
	mov     ax, 3508h                   ; get the timer vector
	int     21h
	mov     word ptr _OldTimerHandler, bx
	mov     word ptr _OldTimerHandler+2, es
	lds     dx, _CalibrationHandler
	mov     ax, 2508h                   ; sample while counting
	int     21h
	pop     ds
	push    ds
	mov     ax, 0FFFFh
	push    ax
	push    ax
	call    _SpinCount
	add     sp, 4
	lds     dx, _OldTimerHandler
	mov     ax, 2508h                   ; and put the old handler back
	int     21h
	pop     ds
	push    ds
	mov     cx, 3
	mov     bx, 1
@@largest:
	mov     si, offset _TickSamples     ; reloaded each pass: all three compare the first two samples
	mov     ax, [si]
	mov     dx, [si+2]
	sub     ax, [si+4]
	sbb     dx, [si+6]
	mov     al, ah
	mov     ah, dl
	cmp     bx, ax
	jae     @@kept
	mov     bx, ax
@@kept:
	loop    @@largest
	mov     _SpinsPerTick, bx
@@calibrated:
	pop     ds
	pop     di
	pop     si
	ret
_CalibrateDelay ENDP

; Wait duration / 65536 timer ticks.
_WaitTicks  PROC FAR
	ARG     duration:DWORD
	push    bp
	mov     bp, sp
	mov     ax, _SpinsPerTick
	mul     word ptr duration
	mov     bl, ah
	mov     bh, dl
	mov     cl, dh
	xor     ch, ch
	mov     ax, _SpinsPerTick
	mul     word ptr duration+2
	add     bh, al
	adc     cl, ah
	adc     ch, dl
	push    cx
	push    bx
	call    _SpinCount
	add     sp, 4
	pop     bp
	ret
_WaitTicks  ENDP

	END
