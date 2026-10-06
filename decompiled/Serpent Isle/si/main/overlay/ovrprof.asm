; Serpent Isle SI.EXE, resident segment 186 (file offsets 0x04010e to 0x0404a5, 919 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.
; Folder chosen by subsystem.

	.MODEL  MEDIUM
	.386

; Overlay profiling keeps its counters in the user interrupt vectors F0h-F4h.
CLOCK_COUNT                EQU 3C0h        ; counter advanced by the periodic interrupt
LOG_NEXT                EQU 3C4h        ; where the next log entry goes
LOG_FLAGS               EQU 3C8h        ; low byte: logging on; high byte: mark each load on screen
LOG_COUNT               EQU 3CCh        ; log entries written
LOAD_COUNT              EQU 3D0h        ; overlay loads
RTC_VECTOR              EQU 70h * 4     ; IRQ 8, the clock chip
OVERLAY_VECTOR          EQU 3Fh * 4     ; INT 3Fh, the overlay manager

	PUBLIC  _CallOverlayManager, _StartOverlayClock, _HookOverlayLoads, _UnhookOverlayLoads
	PUBLIC  _StopOverlayClock, _ClearOverlayClock, _SetOverlayClock, _ReadOverlayClock
	PUBLIC  _MarkOverlayLoads, _LogOverlayLoads, _LogAndMarkOverlayLoads, _GetOverlayLogCount
	PUBLIC  _GetOverlayLoadCount, _LogOverlayMarker

	.DATA
saved_clock             dd  0           ; the vector contents the counters replace
saved_log_next          dd  0
saved_log_flags         dd  0
saved_log_count         dd  0
saved_load_count        dd  0
rtc_a                   dw  0           ; clock registers A to D, as last read
rtc_b                   dw  0
rtc_c                   dw  0
rtc_d                   dw  0

	.CODE

old_rtc                 dd  0           ; the clock handler before ours
old_ovr                 dd  0           ; the overlay manager's INT 3Fh handler
saved_eax               dd  0

; Calls the overlay manager directly, keeping every register.
_CallOverlayManager     PROC FAR
	push    eax
	push    ebx
	push    ecx
	push    edx
	push    edi
	push    esi
	push    ds
	push    es
	int     3Fh
	pop     es
	pop     ds
	pop     esi
	pop     edi
	pop     edx
	pop     ecx
	pop     ebx
	pop     eax
	ret
_CallOverlayManager     ENDP

; Starts the clock: hooks and unmasks IRQ 8 and turns on the clock chip's periodic interrupt.
_StartOverlayClock      PROC FAR
	push    eax
	push    edi
	push    esi
	push    ds
	pushf
	cli
	cld
	xor     eax, eax
	mov     ds, ax
	mov     si, RTC_VECTOR
	mov     di, offset rtc_irq
	in      al, 0A1h
	test    al, 1
	je      irq8_enabled
	and     al, 0FEh
	out     0A1h, al
irq8_enabled:
	mov     eax, dword ptr [si]
	mov     word ptr [si+2], cs
	mov     word ptr [si], di
	mov     dword ptr cs:old_rtc, eax
	mov     si, CLOCK_COUNT
	mov     eax, dword ptr [si]
	mov     dword ptr ss:saved_clock, eax
	xor     eax, eax
	mov     dword ptr [si], eax
	mov     al, 0Ah
	out     70h, al
	in      al, 71h
	and     al, 0F0h
	or      al, 3
	mov     bx, ax
	mov     al, 0Ah
	out     70h, al
	mov     ax, bx
	out     71h, al
	xor     ax, ax                  ; give the clock chip time
	xor     ax, ax
	xor     ax, ax
	xor     ax, ax
	xor     ax, ax
	xor     ax, ax
	xor     ax, ax
	xor     ax, ax
	mov     al, 0Ah
	out     70h, al
	in      al, 71h
	mov     al, 0Bh
	out     70h, al
	in      al, 71h
	and     al, 7
	or      al, 40h
	mov     bx, ax
	mov     ax, 0Bh
	out     70h, al
	mov     ax, bx
	out     71h, al
	popf
	pop     ds
	pop     esi
	pop     edi
	pop     eax
	ret
_StartOverlayClock      ENDP

; Starts counting overlay loads, logging into the buffer at the linear address given.
_HookOverlayLoads       PROC FAR
	enter   0, 0
	push    edi
	push    esi
	push    ds
	pushf
	cli
	xor     eax, eax
	mov     ds, ax
	mov     di, offset ovr_hook
	mov     si, OVERLAY_VECTOR
	mov     eax, dword ptr [si]
	mov     word ptr [si+2], cs
	mov     word ptr [si], di
	mov     dword ptr cs:old_ovr, eax
	mov     si, LOG_NEXT
	mov     eax, dword ptr [si]
	mov     dword ptr ss:saved_log_next, eax
	mov     eax, dword ptr [bp+6]
	mov     dword ptr [si], eax
	mov     si, LOG_FLAGS
	mov     eax, dword ptr [si]
	mov     dword ptr ss:saved_log_flags, eax
	xor     eax, eax
	mov     dword ptr [si], eax
	mov     si, LOG_COUNT
	mov     eax, dword ptr [si]
	mov     dword ptr ss:saved_log_count, eax
	xor     eax, eax
	mov     dword ptr [si], eax
	mov     si, LOAD_COUNT
	mov     eax, dword ptr [si]
	mov     dword ptr ss:saved_load_count, eax
	xor     eax, eax
	mov     dword ptr [si], eax
	popf
	pop     ds
	pop     esi
	pop     edi
	leave
	ret
_HookOverlayLoads       ENDP

; Puts back the overlay manager's vector and what the counters replaced.
_UnhookOverlayLoads     PROC FAR
	pushf
	cli
	xor     eax, eax
	mov     ds, ax
	mov     si, OVERLAY_VECTOR
	mov     eax, dword ptr cs:old_ovr
	mov     dword ptr [si], eax
	mov     si, LOG_NEXT
	mov     eax, dword ptr ss:saved_log_next
	mov     dword ptr [si], eax
	mov     si, LOG_FLAGS
	mov     eax, dword ptr ss:saved_log_flags
	mov     dword ptr [si], eax
	mov     si, LOG_COUNT
	mov     eax, dword ptr ss:saved_log_count
	mov     dword ptr [si], eax
	mov     si, LOAD_COUNT
	mov     eax, dword ptr ss:saved_load_count
	mov     dword ptr [si], eax
	popf
	ret
_UnhookOverlayLoads     ENDP

; Counts an overlay load, logs its return address and marks the screen's corner as the flags ask,
; then goes on to the overlay manager.
ovr_hook                PROC FAR
	mov     dword ptr cs:saved_eax, eax
	pop     eax
	push    eax
	push    ebx
	push    esi
	push    ds
	xor     ebx, ebx
	mov     ds, bx
	mov     esi, LOAD_COUNT
	mov     ebx, dword ptr [esi]
	inc     ebx
	mov     dword ptr [esi], ebx
	mov     esi, LOG_FLAGS
	mov     ebx, dword ptr [esi]
	or      bl, bl
	je      not_logging
	mov     esi, LOG_NEXT
	mov     ebx, dword ptr [esi]
	mov     dword ptr [ebx], eax
	add     ebx, 4
	mov     dword ptr [esi], ebx
	mov     esi, LOG_COUNT
	mov     ebx, dword ptr [esi]
	inc     ebx
	mov     dword ptr [esi], ebx
not_logging:
	or      bh, bh
	je      no_mark
	mov     ebx, 0A0000h
	mov     word ptr [ebx], 1414h
no_mark:
	pop     ds
	pop     esi
	pop     ebx
	mov     eax, dword ptr cs:saved_eax
	jmp     dword ptr cs:old_ovr
ovr_hook                ENDP

; Adds 10 to the clock on each periodic interrupt.
rtc_irq                 PROC FAR
	push    eax
	mov     al, 0Ch
	out     70h, al
	in      al, 71h
	test    al, 40h
	je      not_periodic
	push    si
	push    ds
	xor     eax, eax
	mov     ds, ax
	mov     si, CLOCK_COUNT
	mov     eax, dword ptr [si]
	add     eax, 10
	mov     dword ptr [si], eax
	pop     ds
	pop     si
not_periodic:
	mov     al, 20h
	out     0A0h, al
	out     20h, al
	pop     eax
	iret
rtc_irq                 ENDP

; Puts back the clock handler and what the clock replaced, masks IRQ 8 and ends the periodic interrupt.
_StopOverlayClock       PROC FAR
	push    eax
	push    ebx
	push    edx
	push    si
	push    ds
	pushf
	cli
	mov     eax, dword ptr cs:old_rtc
	mov     edx, dword ptr ss:saved_clock
	xor     bx, bx
	mov     ds, bx
	mov     si, RTC_VECTOR
	mov     dword ptr [si], eax
	mov     si, CLOCK_COUNT
	mov     dword ptr [si], edx
	in      al, 0A1h
	test    al, 1
	jne     irq8_masked
	or      al, 1
	out     0A1h, al
irq8_masked:
	mov     al, 0Bh
	out     70h, al
	in      al, 71h
	and     al, 0BFh
	mov     bx, ax
	mov     ax, 0Bh
	out     70h, al
	mov     ax, bx
	out     71h, al
	popf
	pop     ds
	pop     si
	pop     edx
	pop     ebx
	pop     eax
	ret
_StopOverlayClock       ENDP

; Sets the clock to 0.
_ClearOverlayClock      PROC FAR
	push    si
	push    ds
	pushf
	cli
	xor     eax, eax
	mov     ds, ax
	mov     si, CLOCK_COUNT
	mov     dword ptr [si], eax
	popf
	pop     ds
	pop     si
	ret
_ClearOverlayClock      ENDP

; Sets the clock to the long given.
_SetOverlayClock        PROC FAR
	enter   0, 0
	push    di
	push    ds
	pushf
	cli
	xor     eax, eax
	mov     ds, ax
	mov     eax, dword ptr [bp+6]
	mov     si, CLOCK_COUNT
	mov     dword ptr [si], eax
	popf
	pop     ds
	pop     di
	leave
	ret
_SetOverlayClock        ENDP

; Keeps the clock chip's registers A to D and returns the clock in DX:AX.
_ReadOverlayClock       PROC FAR
	push    si
	push    ds
	pushf
	cli
	xor     eax, eax
	mov     al, 0Ah
	out     70h, al
	in      al, 71h
	mov     word ptr rtc_a, ax
	xor     eax, eax
	mov     al, 0Bh
	out     70h, al
	in      al, 71h
	mov     word ptr rtc_b, ax
	xor     eax, eax
	mov     al, 0Ch
	out     70h, al
	in      al, 71h
	mov     word ptr rtc_c, ax
	xor     eax, eax
	mov     al, 0Dh
	out     70h, al
	in      al, 71h
	mov     word ptr rtc_d, ax
	xor     eax, eax
	mov     ds, ax
	mov     si, CLOCK_COUNT
	mov     eax, dword ptr [si]
	push    eax
	xor     eax, eax
	pop     ax
	pop     dx
	popf
	pop     ds
	pop     si
	ret
_ReadOverlayClock       ENDP

; Marks each overlay load on screen as well.
_MarkOverlayLoads       PROC FAR
	push    si
	push    ds
	pushf
	cli
	xor     eax, eax
	mov     ds, ax
	mov     si, LOG_FLAGS
	mov     eax, dword ptr [si]
	or      eax, 100h
	mov     dword ptr [si], eax
	popf
	pop     ds
	pop     si
	ret
_MarkOverlayLoads       ENDP

; Logs overlay loads, without marking them.
_LogOverlayLoads        PROC FAR
	push    si
	push    ds
	pushf
	cli
	xor     eax, eax
	mov     ds, ax
	mov     si, LOG_FLAGS
	mov     eax, 1
	mov     dword ptr [si], eax
	popf
	pop     ds
	pop     si
	ret
_LogOverlayLoads        ENDP

; Logs and marks overlay loads.
_LogAndMarkOverlayLoads PROC FAR
	push    si
	push    ds
	pushf
	cli
	xor     eax, eax
	mov     ds, ax
	mov     si, LOG_FLAGS
	mov     eax, 0FFFFFFFFh
	mov     dword ptr [si], eax
	popf
	pop     ds
	pop     si
	ret
_LogAndMarkOverlayLoads ENDP

; Returns the number of log entries in DX:AX.
_GetOverlayLogCount     PROC FAR
	push    si
	push    ds
	pushf
	cli
	xor     eax, eax
	mov     ds, ax
	mov     si, LOG_COUNT
	mov     eax, dword ptr [si]
	push    eax
	pop     ax
	pop     dx
	popf
	pop     ds
	pop     si
	ret
_GetOverlayLogCount     ENDP

; Returns the number of overlay loads in DX:AX.
_GetOverlayLoadCount    PROC FAR
	push    si
	push    ds
	pushf
	cli
	xor     eax, eax
	mov     ds, ax
	mov     si, LOAD_COUNT
	mov     eax, dword ptr [si]
	push    eax
	pop     ax
	pop     dx
	popf
	pop     ds
	pop     si
	ret
_GetOverlayLoadCount    ENDP

; Logs a marker: FFFFh in the low word and the number given in the high word.
_LogOverlayMarker       PROC FAR
	enter   0, 0
	push    eax
	push    ebx
	push    esi
	push    ds
	pushf
	cli
	xor     eax, eax
	mov     ds, ax
	mov     ax, word ptr [bp+6]
	push    ax
	mov     ax, 0FFFFh
	push    ax
	pop     eax
	mov     esi, LOG_NEXT
	mov     ebx, dword ptr [esi]
	mov     dword ptr [ebx], eax
	add     ebx, 4
	mov     dword ptr [esi], ebx
	mov     esi, LOG_COUNT
	mov     ebx, dword ptr [esi]
	inc     ebx
	mov     dword ptr [esi], ebx
	popf
	pop     ds
	pop     esi
	pop     ebx
	pop     eax
	leave
	ret
_LogOverlayMarker       ENDP

	END
