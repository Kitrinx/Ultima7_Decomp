; Serpent Isle SI.EXE, one module of resident segment 58 (file offsets 0x023300 to 0x02337a, 122 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

; Holds a view's row address table.

	.MODEL  MEDIUM
	.386
	LOCALS

	PUBLIC  _SetRowAddress, _GetRowAddress

	EXTRN   _EnterFlatMode:FAR

	.DATA
	EXTRN   _FlatModeFlags:WORD

LOWLEVEL_TEXT SEGMENT DWORD PUBLIC USE16 'CODE'
	ASSUME  cs:LOWLEVEL_TEXT

; Store address as entry row of a flat table of row addresses.
_SetRowAddress PROC FAR
	ARG     row:WORD, address:DWORD, table:DWORD
	enter   0, 0
	push    edi
	push    es
	pushf
	cld
	mov     ax, word ptr _FlatModeFlags
	and     ax, 1
	je      @@start
	call    far ptr _EnterFlatMode
@@start:
	xor     eax, eax
	mov     es, ax
	mov     edi, table
	movzx   eax, word ptr row
	shl     eax, 2
	add     edi, eax
	mov     eax, address
	stos    dword ptr es:[edi]
	popf
	pop     es
	pop     edi
	leave
	ret
_SetRowAddress ENDP

; Return entry row of a flat table of row addresses in dx:ax.
_GetRowAddress PROC FAR
	ARG     row:WORD, table:DWORD
	enter   0, 0
	push    esi
	push    ds
	pushf
	cld
	mov     ax, word ptr _FlatModeFlags
	and     ax, 1
	je      @@start
	call    far ptr _EnterFlatMode
@@start:
	xor     eax, eax
	xor     edx, edx
	mov     ds, ax
	mov     esi, table
	movzx   eax, word ptr row
	shl     eax, 2
	add     esi, eax
	lods    dword ptr [esi]
	push    eax
	xor     eax, eax
	pop     ax
	pop     dx
	popf
	pop     ds
	pop     esi
	leave
	ret
_GetRowAddress ENDP

LOWLEVEL_TEXT ENDS

	END
