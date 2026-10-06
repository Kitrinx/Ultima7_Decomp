; Serpent Isle SI.EXE, one module of resident segment 58 (file offsets 0x0245fc to 0x0246b2, 182 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

; Holds flat-to-flat memory moves.

	.MODEL  MEDIUM
	.386
	LOCALS

	PUBLIC  _MoveLinearFlat

	EXTRN   _EnterFlatMode:FAR

	.DATA
	EXTRN   _FlatModeFlags:WORD

LOWLEVEL_TEXT SEGMENT DWORD PUBLIC USE16 'CODE'
	ASSUME  cs:LOWLEVEL_TEXT

; Copy bytes between two flat addresses, safe for overlap:
; copies backward when the target lies inside the source.
_MoveLinearFlat PROC FAR
	ARG     dst:DWORD, src:DWORD, bytes:DWORD
	enter   0, 0
	push    edi
	push    esi
	push    ds
	push    es
	mov     ax, word ptr _FlatModeFlags
	and     ax, 1
	je      @@ready
	call    far ptr _EnterFlatMode
@@ready:
	pushf
	mov     ecx, bytes
	mov     edi, dst
	mov     esi, src
	cld
	cmp     edi, esi
	jl      @@forward
	je      @@done
	push    esi
	add     esi, ecx
	cmp     edi, esi
	pop     esi
	jg      @@forward
	std
	cmp     ecx, 4
	jge     @@backDwords
	add     edi, ecx
	dec     edi
	add     esi, ecx
	dec     esi
	jmp     @@backBytes
; backward: odd bytes first, then dwords
@@backDwords:
	add     edi, ecx
	dec     edi
	add     esi, ecx
	dec     esi
	xor     eax, eax
	mov     es, ax
	mov     ds, ax
	mov     eax, ecx
	and     ecx, 3
	rep     movs byte ptr es:[edi], byte ptr [esi]
	sub     esi, 3
	sub     edi, 3
	mov     ecx, eax
	shr     ecx, 2
	rep     movs dword ptr es:[edi], dword ptr [esi]
	jmp     @@done
; forward: odd bytes first, then dwords
@@forward:
	xor     eax, eax
	mov     es, ax
	mov     ds, ax
	mov     eax, ecx
	and     ecx, 3
	rep     movs byte ptr es:[edi], byte ptr [esi]
	mov     ecx, eax
	shr     ecx, 2
	rep     movs dword ptr es:[edi], dword ptr [esi]
	jmp     @@done
@@backBytes:
	and     ecx, 3
	rep     movs byte ptr es:[edi], byte ptr [esi]
@@done:
	popf
	pop     es
	pop     ds
	pop     esi
	pop     edi
	leave
	ret
_MoveLinearFlat ENDP

LOWLEVEL_TEXT ENDS

	END
