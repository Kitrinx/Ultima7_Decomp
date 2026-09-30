; Black Gate U7.EXE, resident segment 140 (file offsets 0x03f026 to 0x03f099, 115 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.
; Folder inferred from compiler flags and link order.

	.MODEL  MEDIUM
	.386
	LOCALS

	PUBLIC  _ClearFlatBit, _SetFlatBit, _TestFlatBit

	.CODE

; Bit operations on a dword at a linear address, reached through a zero segment.

; Clears the bit.
_ClearFlatBit   PROC FAR
	ARG     linear:DWORD, bit:WORD
	enter   0, 0
	push    esi
	push    ds
	mov     esi, linear
	xor     eax, eax
	mov     ds, ax
	mov     eax, [esi]
	movzx   ecx, bit
	btr     eax, ecx
	mov     [esi], eax
	pop     ds
	pop     esi
	leave
	ret
_ClearFlatBit   ENDP

; Sets the bit.
_SetFlatBit PROC FAR
	ARG     linear:DWORD, bit:WORD
	enter   0, 0
	push    esi
	push    ds
	mov     esi, linear
	xor     eax, eax
	mov     ds, ax
	mov     eax, [esi]
	movzx   ecx, bit
	bts     eax, ecx
	mov     [esi], eax
	pop     ds
	pop     esi
	leave
	ret
_SetFlatBit ENDP

; Tests the bit: -1 when set, 0 when clear.
_TestFlatBit    PROC FAR
	ARG     linear:DWORD, bit:WORD
	enter   0, 0
	push    esi
	push    ds
	mov     esi, linear
	xor     eax, eax
	mov     ds, ax
	mov     ebx, [esi]
	movzx   ecx, bit
	bt      ebx, ecx
	jnc     @@done
	dec     ax
@@done:
	pop     ds
	pop     esi
	leave
	ret
_TestFlatBit    ENDP

	END
