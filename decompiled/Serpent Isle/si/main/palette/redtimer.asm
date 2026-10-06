; Serpent Isle SI.EXE, resident segment 94 (file offsets 0x0351fa to 0x03530e, 276 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

	.MODEL  MEDIUM
	.386

TIMER_VECTOR            EQU 8 * 4       ; INT 8, the BIOS timer tick

	PUBLIC  _RedScreenStepTicks, _UnhookRedScreenTimer, _StartRedScreenCycle, _StopRedScreenCycle, _HookRedScreenTimer
	GLOBAL  @CycleRedScreenPalette$qv:FAR

	.DATA
countdown               dd  0           ; timer ticks until the next palette step
_RedScreenStepTicks     dd  1           ; ticks between palette steps
cycling                 dw  0           ; nonzero while the palette cycles
step_entry              dd  0           ; far address of palette_tick

	.CODE

old_timer               dd  0           ; the timer handler this one chains to
saved_eax               dd  0
tick_pending            dw  0           ; palette_tick has been scheduled
tick_done               dw  0           ; palette_tick has run since

; Timer handler: once per tick, while cycling, arranges for the old handler's IRET to land in
; palette_tick, so the palette steps after the BIOS has finished with the interrupt.
timer_hook              PROC FAR
	pushf
	cli
	mov     cs:saved_eax, eax
	push    ds
	mov     ax, DGROUP
	mov     ds, ax
	mov     ax, cycling
	or      ax, ax
	je      chain
	mov     ax, cs:tick_done
	or      ax, ax
	je      not_done
	xor     ax, ax
	mov     cs:tick_pending, ax
	mov     cs:tick_done, ax
	jmp     chain
not_done:
	mov     ax, cs:tick_pending
	or      ax, ax
	jne     chain
	mov     ax, 1
	mov     cs:tick_pending, ax
	mov     eax, step_entry
	pop     ds
	push    eax
	mov     eax, cs:saved_eax
	jmp     to_old
chain:
	pop     ds
	mov     eax, cs:saved_eax
	popf
to_old:
	jmp     cs:old_timer
timer_hook              ENDP

; Counts down and steps the palette when the count runs out.
palette_tick            PROC FAR
	pushf
	sti
	push    eax
	push    ds
	push    es
	mov     ax, DGROUP
	mov     ds, ax
	mov     ax, cycling
	or      ax, ax
	je      tick_exit
	mov     eax, countdown
	dec     eax
	mov     countdown, eax
	jne     tick_exit
	push    ds
	pop     es
	mov     eax, _RedScreenStepTicks
	mov     countdown, eax
	pushad
	push    ds
	push    es
	call    far ptr @CycleRedScreenPalette$qv
	pop     es
	pop     ds
	popad
tick_exit:
	pop     es
	pop     ds
	pop     eax
	popf
	cli
	mov     cs:tick_done, 1
	iret
palette_tick            ENDP

; Puts the old timer handler back.
_UnhookRedScreenTimer   PROC FAR
	push    si
	push    ds
	mov     si, TIMER_VECTOR
	xor     ax, ax
	mov     ds, ax
	mov     eax, cs:old_timer
	pushf
	cli
	mov     [si], eax
	popf
	xor     ax, ax
	pop     ds
	pop     si
	ret
_UnhookRedScreenTimer   ENDP

; Starts cycling.
_StartRedScreenCycle    PROC FAR
	mov     ax, 1
	mov     cycling, ax
	ret
_StartRedScreenCycle    ENDP

; Stops cycling.
_StopRedScreenCycle     PROC FAR
	xor     ax, ax
	mov     cycling, ax
	ret
_StopRedScreenCycle     ENDP

; Hooks the timer, not yet cycling.
_HookRedScreenTimer     PROC FAR
	push    eax
	push    edi
	push    esi
	push    ds
	mov     eax, _RedScreenStepTicks
	mov     countdown, eax
	xor     eax, eax
	mov     cycling, ax
	mov     word ptr step_entry+2, cs
	mov     di, offset palette_tick
	mov     word ptr step_entry, di
	mov     ds, ax
	mov     si, TIMER_VECTOR
	mov     eax, [si]
	mov     cs:old_timer, eax
	mov     di, offset timer_hook
	pushf
	cli
	push    cs
	push    di
	pop     eax
	mov     [si], eax
	popf
	pop     ds
	pop     esi
	pop     edi
	pop     eax
	ret
_HookRedScreenTimer     ENDP

	END
