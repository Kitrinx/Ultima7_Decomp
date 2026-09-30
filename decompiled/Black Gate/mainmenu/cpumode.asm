; Black Gate MAINMENU.EXE, resident segment 61 (file offsets 0x018b22 to 0x018b4c, 42 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

	.MODEL  MEDIUM
	.386P
	LOCALS

	PUBLIC  _GetProcessorMode

CR0_PE          EQU 1                   ; protection enable
EFL_AC          EQU 40000h              ; alignment check

	.CODE

; Return 2 in protected or virtual 8086 mode, 1 when the alignment check flag
; is set, else 0.
_GetProcessorMode PROC FAR
	pushf
	smsw    ax
	and     eax, CR0_PE
	je      @@flags
	mov     ax, 2
	jmp     @@done
@@flags:
	pushfd
	pop     eax
	test    eax, EFL_AC
	je      @@real
	mov     ax, 1
	jmp     @@done
@@real:
	xor     ax, ax
@@done:
	popf
	ret
_GetProcessorMode ENDP

	END
