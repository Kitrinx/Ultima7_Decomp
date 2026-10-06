; Serpent Isle INTRO.EXE, resident segment 76 (file offsets 0x014c8c to 0x015498, 2060 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

	.MODEL  MEDIUM
	.386
	LOCALS

	PUBLIC  _SumLinearBytes, _CopyLinearStringN, _CopyLinearString, _PeekLong
	PUBLIC  _PokeLong, _PeekWord, _PokeWord, _PeekByte, _PokeByte, _TestLinearBit, _SetLinearBit
	PUBLIC  _ClearLinearBit, _FillLinear, _FillLinearRect, _MoveLinear, _CopyFarToLinear
	PUBLIC  _CopyLinearToFar, _FillScreen, _CopyScreen, _CopyScreenWords, _ClearScreen, _RemapScreen
	PUBLIC  _FillScreenBuffer

	EXTRN   _EnterFlatMode:FAR

SCREEN_WIDTH    EQU 320
SCREEN_HEIGHT   EQU 200


	.DATA
	EXTRN   _FlatModeFlags:WORD

LOWLEVEL_TEXT SEGMENT DWORD PUBLIC USE16 'CODE'
	ASSUME  cs:LOWLEVEL_TEXT

; Return the sum of numBytes bytes at a flat address in dx:ax.
_SumLinearBytes PROC FAR
	ARG     src:DWORD, numBytes:DWORD
	enter   0, 0
	push    esi
	push    ds
	cld
	mov     ax, word ptr _FlatModeFlags
	and     ax, 1
	je      @@start
	call    far ptr _EnterFlatMode
@@start:
	mov     esi, src
	mov     ecx, numBytes
	xor     eax, eax
	xor     ebx, ebx
	mov     ds, ax
	or      ecx, ecx
	je      @@done
@@add:
	mov     al, [esi]
	inc     esi
	add     ebx, eax
	dec     ecx
	jne     @@add
@@done:
	xor     eax, eax
	xor     edx, edx
	push    ebx
	pop     ax
	pop     dx
	pop     ds
	pop     esi
	leave
	ret
_SumLinearBytes ENDP

; Copy a string of at most maxLen bytes. Bit 0 of flags marks src
; as flat and bit 8 dest; otherwise each is a far pointer.
_CopyLinearStringN PROC FAR
	ARG     dest:DWORD, src:DWORD, maxLen:WORD, flags:WORD
	enter   0, 0
	push    edi
	push    esi
	push    ds
	push    es
	cld
	mov     ax, word ptr _FlatModeFlags
	and     ax, 1
	je      @@start
	call    far ptr _EnterFlatMode
@@start:
	mov     ax, flags
	and     ax, 1
	mov     eax, src
	jne     @@srcFlat
	xor     edx, edx
	push    0
	push    eax
	pop     dx
	pop     eax
	shl     eax, 4
	add     eax, edx
@@srcFlat:
	mov     esi, eax
	mov     ax, flags
	and     ax, 100h
	mov     eax, dest
	jne     @@destFlat
	xor     edx, edx
	push    0
	push    eax
	pop     dx
	pop     eax
	shl     eax, 4
	add     eax, edx
@@destFlat:
	mov     edi, eax
	xor     eax, eax
	mov     ds, ax
	mov     es, ax
	movzx   edx, word ptr maxLen
@@copy:
	lods    byte ptr [esi]
	stos    byte ptr es:[edi]
	dec     edx
	je      @@done
	or      al, al
	jne     @@copy
@@done:
	pop     es
	pop     ds
	pop     esi
	pop     edi
	leave
	ret
_CopyLinearStringN ENDP

; Copy a string. Bit 0 of flags marks src as flat and bit 8 dest;
; otherwise each is a far pointer.
_CopyLinearString PROC FAR
	ARG     dest:DWORD, src:DWORD, flags:WORD
	enter   0, 0
	push    edi
	push    esi
	push    ds
	push    es
	cld
	mov     ax, word ptr _FlatModeFlags
	and     ax, 1
	je      @@start
	call    far ptr _EnterFlatMode
@@start:
	mov     ax, flags
	and     ax, 1
	mov     eax, src
	jne     @@srcFlat
	xor     edx, edx
	push    0
	push    eax
	pop     dx
	pop     eax
	shl     eax, 4
	add     eax, edx
@@srcFlat:
	mov     esi, eax
	mov     ax, flags
	and     ax, 100h
	mov     eax, dest
	jne     @@destFlat
	xor     edx, edx
	push    0
	push    eax
	pop     dx
	pop     eax
	shl     eax, 4
	add     eax, edx
@@destFlat:
	mov     edi, eax
	xor     eax, eax
	mov     ds, ax
	mov     es, ax
@@copy:
	lods    byte ptr [esi]
	stos    byte ptr es:[edi]
	or      al, al
	jne     @@copy
	pop     es
	pop     ds
	pop     esi
	pop     edi
	leave
	ret
_CopyLinearString ENDP

; Return the dword at a flat address in dx:ax.
_PeekLong PROC FAR
	ARG     address:DWORD
	enter   0, 0
	push    esi
	push    ds
	cld
	mov     ax, word ptr _FlatModeFlags
	and     ax, 1
	je      @@start
	call    far ptr _EnterFlatMode
@@start:
	mov     esi, address
	xor     eax, eax
	mov     ds, ax
	xor     eax, eax
	xor     edx, edx
	lods    dword ptr [esi]
	push    eax
	pop     ax
	pop     dx
	pop     ds
	pop     esi
	leave
	ret
_PeekLong ENDP

; Store a dword at a flat address.
_PokeLong PROC FAR
	ARG     address:DWORD, value:DWORD
	enter   0, 0
	push    edi
	push    es
	cld
	mov     ax, word ptr _FlatModeFlags
	and     ax, 1
	je      @@start
	call    far ptr _EnterFlatMode
@@start:
	mov     edi, address
	xor     eax, eax
	mov     es, ax
	mov     eax, value
	stos    dword ptr es:[edi]
	pop     es
	pop     edi
	leave
	ret
_PokeLong ENDP

; Return the word at a flat address.
_PeekWord PROC FAR
	ARG     address:DWORD
	enter   0, 0
	push    esi
	push    ds
	cld
	mov     ax, word ptr _FlatModeFlags
	and     ax, 1
	je      @@start
	call    far ptr _EnterFlatMode
@@start:
	mov     esi, address
	xor     eax, eax
	mov     ds, ax
	xor     eax, eax
	lods    word ptr [esi]
	pop     ds
	pop     esi
	leave
	ret
_PeekWord ENDP

; Store a word at a flat address.
_PokeWord PROC FAR
	ARG     address:DWORD, value:WORD
	enter   0, 0
	push    edi
	push    es
	cld
	mov     ax, word ptr _FlatModeFlags
	and     ax, 1
	je      @@start
	call    far ptr _EnterFlatMode
@@start:
	mov     edi, address
	xor     eax, eax
	mov     es, ax
	mov     ax, value
	stos    word ptr es:[edi]
	pop     es
	pop     edi
	leave
	ret
_PokeWord ENDP

; Return the byte at a flat address.
_PeekByte PROC FAR
	ARG     address:DWORD
	enter   0, 0
	push    esi
	push    ds
	cld
	mov     ax, word ptr _FlatModeFlags
	and     ax, 1
	je      @@start
	call    far ptr _EnterFlatMode
@@start:
	mov     esi, address
	xor     eax, eax
	mov     ds, ax
	xor     eax, eax
	lods    byte ptr [esi]
	pop     ds
	pop     esi
	leave
	ret
_PeekByte ENDP

; Store a byte at a flat address.
_PokeByte PROC FAR
	ARG     address:DWORD, value:BYTE
	enter   0, 0
	push    edi
	push    es
	cld
	mov     ax, word ptr _FlatModeFlags
	and     ax, 1
	je      @@start
	call    far ptr _EnterFlatMode
@@start:
	mov     edi, address
	xor     eax, eax
	mov     es, ax
	mov     al, value
	stos    byte ptr es:[edi]
	pop     es
	pop     edi
	leave
	ret
_PokeByte ENDP

; Test bit n, counted from 1, of a bit array at a flat address.
; Returns 1 when set, else 0.
_TestLinearBit PROC FAR
	ARG     bits:DWORD, bitNum:WORD
	enter   0, 0
	push    esi
	push    ds
	cld
	mov     ax, word ptr _FlatModeFlags
	and     ax, 1
	je      @@ready
	call    far ptr _EnterFlatMode
@@ready:
	mov     esi, bits
	xor     eax, eax
	mov     ds, ax
	movzx   eax, word ptr bitNum
	dec     eax
	mov     ecx, eax
	and     ecx, 7
	inc     ecx
	shr     eax, 3
	add     esi, eax
	xor     eax, eax
	lods    byte ptr [esi]
	dec     ecx
	bt      ax, cx
	rcl     ax, 1
	and     eax, 1
	pop     ds
	pop     esi
	leave
	ret
_TestLinearBit ENDP

; Set bit n, counted from 1, of a bit array at a flat address.
_SetLinearBit PROC FAR
	ARG     bits:DWORD, bitNum:WORD
	enter   0, 0
	push    edi
	push    es
	cld
	mov     ax, word ptr _FlatModeFlags
	and     ax, 1
	je      @@ready
	call    far ptr _EnterFlatMode
@@ready:
	mov     edi, bits
	xor     eax, eax
	mov     es, ax
	movzx   eax, word ptr bitNum
	dec     eax
	mov     ecx, eax
	and     ecx, 7
	inc     ecx
	shr     eax, 3
	add     edi, eax
	xor     eax, eax
	mov     al, [edi]
	dec     ecx
	bts     ax, cx
	stos    byte ptr es:[edi]
	pop     es
	pop     edi
	leave
	ret
_SetLinearBit ENDP

; Clear a bit of a bit array at a flat address. Unlike the test and set
; routines it counts from 0 and steps sixteen bits per byte.
_ClearLinearBit PROC FAR
	ARG     bits:DWORD, bitNum:WORD
	enter   0, 0
	push    edi
	push    es
	cld
	mov     ax, word ptr _FlatModeFlags
	and     ax, 1
	je      @@ready
	call    far ptr _EnterFlatMode
@@ready:
	mov     edi, bits
	xor     eax, eax
	mov     es, ax
	movzx   eax, word ptr bitNum
	mov     ecx, eax
	and     ecx, 7
	shr     eax, 4
	add     edi, eax
	xor     eax, eax
	mov     al, [edi]
	dec     ecx
	btr     ax, cx
	stos    byte ptr es:[edi]
	pop     es
	pop     edi
	leave
	ret
_ClearLinearBit ENDP

; Fill a run of memory with one byte. Flag 100h says the destination
; is already flat; otherwise it is a segment:offset pointer.
_FillLinear PROC FAR
	ARG     dst:DWORD, fillByte:WORD, bytes:DWORD, flags:WORD
	enter   0, 0
	push    edi
	push    es
	cld
	mov     ax, word ptr _FlatModeFlags
	and     ax, 1
	je      @@ready
	call    far ptr _EnterFlatMode
@@ready:
	mov     ecx, bytes
	mov     ax, flags
	and     ax, 100h
	mov     eax, dst
	jne     @@flat
	xor     edx, edx
	push    0
	push    eax
	pop     dx
	pop     eax
	shl     eax, 4
	add     eax, edx
; odd bytes first, then whole dwords
@@flat:
	mov     edi, eax
	xor     eax, eax
	mov     es, ax
	xor     eax, eax
	mov     al, byte ptr fillByte
	mov     ah, al
	push    ax
	push    ax
	pop     eax
	mov     ebx, ecx
	and     ecx, 3
	rep     stos byte ptr es:[edi]
	mov     ecx, ebx
	shr     ecx, 2
	rep     stos dword ptr es:[edi]
	pop     es
	pop     edi
	leave
	ret
_FillLinear ENDP

; Fill a rectangle of rows at a flat address with one byte.
_FillLinearRect PROC FAR
	ARG     dst:DWORD, fillByte:WORD, cols:WORD, pitch:WORD, rows:WORD
	enter   0, 0
	push    esi
	push    edi
	push    es
	cld
	mov     ax, word ptr _FlatModeFlags
	and     ax, 1
	je      @@ready
	call    far ptr _EnterFlatMode
@@ready:
	movzx   ebx, word ptr cols
	movzx   edx, word ptr pitch
	mov     esi, dst
	movzx   ecx, word ptr rows
	xor     eax, eax
	mov     es, ax
	xor     eax, eax
	mov     al, byte ptr fillByte
	mov     ah, al
	push    ax
	push    ax
	pop     eax
@@row:
	push    ecx
	mov     ecx, ebx
	mov     edi, esi
	and     ecx, 3
	rep     stos byte ptr es:[edi]
	mov     ecx, ebx
	shr     ecx, 2
	rep     stos dword ptr es:[edi]
	add     esi, edx
	pop     ecx
	dec     ecx
	jne     @@row
	pop     es
	pop     edi
	pop     esi
	leave
	ret
_FillLinearRect ENDP

; Move bytes between flat or segment:offset addresses. Flag 1 marks
; the source flat and flag 100h the destination.
_MoveLinear PROC FAR
	ARG     dst:DWORD, src:DWORD, bytes:DWORD, flags:WORD
	enter   0, 0
	push    edi
	push    esi
	push    ds
	push    es
	pushf
	cld
	mov     ax, word ptr _FlatModeFlags
	and     ax, 1
	je      @@ready
	call    far ptr _EnterFlatMode
@@ready:
	mov     ecx, bytes
	mov     ax, flags
	and     ax, 1
	mov     eax, src
	jne     @@srcFlat
	xor     edx, edx
	push    0
	push    eax
	pop     dx
	pop     eax
	shl     eax, 4
	add     eax, edx
@@srcFlat:
	mov     esi, eax
	mov     ax, flags
	and     ax, 100h
	mov     eax, dst
	jne     @@dstFlat
	xor     edx, edx
	push    0
	push    eax
	pop     dx
	pop     eax
	shl     eax, 4
	add     eax, edx
; copy backwards only when the destination starts inside the source
@@dstFlat:
	mov     edi, eax
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
	jge     @@backLong
	add     edi, ecx
	dec     edi
	add     esi, ecx
	dec     esi
	jmp     @@backShort
@@backLong:
	add     edi, ecx
	sub     edi, 4
	add     esi, ecx
	sub     esi, 4
@@forward:
	xor     eax, eax
	mov     es, ax
	mov     ds, ax
	mov     ebx, ecx
	shr     ecx, 2
	rep     movs dword ptr es:[edi], dword ptr [esi]
	mov     ecx, ebx
@@backShort:
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
_MoveLinear ENDP

; Copy bytes from a segment:offset pointer to a flat address.
_CopyFarToLinear PROC FAR
	ARG     dst:DWORD, src:DWORD, bytes:DWORD
	enter   0, 0
	push    edi
	push    esi
	push    ds
	push    es
	cld
	mov     ax, word ptr _FlatModeFlags
	and     ax, 1
	je      @@ready
	call    far ptr _EnterFlatMode
@@ready:
	mov     ecx, bytes
	mov     edi, dst
	xor     ebx, ebx
	mov     eax, src
	push    eax
	xor     eax, eax
	pop     bx
	pop     ax
	shl     eax, 4
	add     eax, ebx
	mov     esi, eax
	xor     eax, eax
	mov     es, ax
	mov     ds, ax
	mov     ebx, ecx
	and     ecx, 3
	rep     movs byte ptr es:[edi], byte ptr [esi]
	mov     ecx, ebx
	shr     ecx, 2
	rep     movs dword ptr es:[edi], dword ptr [esi]
	pop     es
	pop     ds
	pop     esi
	pop     edi
	leave
	ret
_CopyFarToLinear ENDP

; Copy bytes from a flat address to a segment:offset pointer.
_CopyLinearToFar PROC FAR
	ARG     dst:DWORD, src:DWORD, bytes:DWORD
	enter   0, 0
	push    edi
	push    esi
	push    ds
	push    es
	cld
	mov     ax, word ptr _FlatModeFlags
	and     ax, 1
	je      @@ready
	call    far ptr _EnterFlatMode
@@ready:
	mov     ecx, bytes
	mov     esi, src
	xor     ebx, ebx
	mov     eax, dst
	push    eax
	xor     eax, eax
	pop     bx
	pop     ax
	shl     eax, 4
	add     eax, ebx
	mov     edi, eax
	xor     eax, eax
	mov     es, ax
	mov     ds, ax
	mov     eax, ecx
	and     ecx, 3
	rep     movs byte ptr es:[edi], byte ptr [esi]
	mov     ecx, eax
	shr     ecx, 2
	rep     movs dword ptr es:[edi], dword ptr [esi]
	pop     es
	pop     ds
	pop     esi
	pop     edi
	leave
	ret
_CopyLinearToFar ENDP

; Fill the 320x200 screen at A000:0 with one color.
_FillScreen PROC FAR
	ARG     color:WORD
	enter   0, 0
	push    edi
	push    es
	cld
	mov     ax, word ptr _FlatModeFlags
	and     ax, 1
	je      @@ready
	call    far ptr _EnterFlatMode
@@ready:
	mov     eax, 0A000h
	shl     eax, 4
	mov     edi, eax
	xor     eax, eax
	mov     es, ax
	mov     ax, color
	mov     ah, al
	push    ax
	push    ax
	pop     eax
	mov     ecx, SCREEN_WIDTH * SCREEN_HEIGHT
	shr     ecx, 2
	rep     stos dword ptr es:[edi]
	pop     es
	pop     edi
	leave
	ret
_FillScreen ENDP

; Copy a whole 320x200 screen between two flat addresses, a dword at a time.
_CopyScreen PROC FAR
	ARG     src:DWORD, dst:DWORD
	enter   0, 0
	push    edi
	push    esi
	push    ds
	push    es
	cld
	mov     ax, word ptr _FlatModeFlags
	and     ax, 1
	je      @@ready
	call    far ptr _EnterFlatMode
@@ready:
	xor     eax, eax
	mov     esi, src
	xor     eax, eax
	mov     edi, dst
	xor     eax, eax
	mov     es, ax
	mov     ds, ax
	mov     ecx, SCREEN_WIDTH * SCREEN_HEIGHT
	shr     ecx, 2
	rep     movs dword ptr es:[edi], dword ptr [esi]
	pop     es
	pop     ds
	pop     esi
	pop     edi
	leave
	ret
_CopyScreen ENDP

; Copy a whole 320x200 screen between two flat addresses, a word at a time.
_CopyScreenWords PROC FAR
	ARG     src:DWORD, dst:DWORD
	enter   0, 0
	push    edi
	push    esi
	push    ds
	push    es
	cld
	mov     ax, word ptr _FlatModeFlags
	and     ax, 1
	je      @@ready
	call    far ptr _EnterFlatMode
@@ready:
	xor     eax, eax
	mov     esi, src
	xor     eax, eax
	mov     edi, dst
	xor     eax, eax
	mov     es, ax
	mov     ds, ax
	mov     ecx, SCREEN_WIDTH * SCREEN_HEIGHT
	shr     ecx, 1
	rep     movs word ptr es:[edi], word ptr [esi]
	pop     es
	pop     ds
	pop     esi
	pop     edi
	leave
	ret
_CopyScreenWords ENDP

; Fill the screen at A000:0 with one color, a row per pass.
_ClearScreen PROC FAR
	ARG     color:WORD
	enter   0, 0
	push    edi
	push    es
	cld
	mov     ax, word ptr _FlatModeFlags
	and     ax, 1
	je      @@ready
	call    far ptr _EnterFlatMode
@@ready:
	mov     edi, 0A0000h
	xor     eax, eax
	mov     es, ax
	mov     ax, color
	mov     ah, al
	push    ax
	push    ax
	pop     eax
	mov     ecx, SCREEN_HEIGHT
@@row:
	REPT    SCREEN_WIDTH / 4
	stos    dword ptr es:[edi]
	ENDM
	dec     cx
	jcxz    @@done
	jmp     @@row
@@done:
	pop     es
	pop     edi
	leave
	ret
_ClearScreen ENDP

; Pass every screen pixel through a 256-byte color table.
; Flag 1 says the table address is flat, else segment:offset.
_RemapScreen PROC FAR
	ARG     table:DWORD, flags:WORD
	enter   0, 0
	push    edi
	push    esi
	push    es
	push    ds
	cld
	mov     ax, word ptr _FlatModeFlags
	and     ax, 1
	je      @@ready
	call    far ptr _EnterFlatMode
@@ready:
	xor     eax, eax
	mov     ds, ax
	mov     es, ax
	xor     ebx, ebx
	xor     ecx, ecx
	mov     cx, flags
	mov     ebx, table
	and     ecx, 1
	jne     @@flat
	push    0
	push    ebx
	pop     ax
	pop     ebx
	shl     ebx, 4
	add     ebx, eax
@@flat:
	mov     esi, 0A0000h
	mov     edi, 0A0000h
	mov     ecx, SCREEN_WIDTH * SCREEN_HEIGHT
	xor     eax, eax
@@pixel:
	lods    byte ptr [esi]
	xlat    byte ptr [ebx]
	stos    byte ptr es:[edi]
	loop    @@pixel
	pop     ds
	pop     es
	pop     esi
	pop     edi
	leave
	ret
_RemapScreen ENDP

; Fill a 320x200 buffer at a flat address with one color.
_FillScreenBuffer PROC FAR
	ARG     buffer:DWORD, color:WORD
	enter   0, 0
	push    edi
	push    es
	cld
	mov     ax, word ptr _FlatModeFlags
	and     ax, 1
	je      @@ready
	call    far ptr _EnterFlatMode
@@ready:
	mov     edi, buffer
	xor     eax, eax
	mov     es, ax
	mov     ax, color
	mov     ah, al
	push    ax
	push    ax
	pop     eax
	mov     ecx, SCREEN_WIDTH * SCREEN_HEIGHT
	shr     ecx, 2
	rep     stos dword ptr es:[edi]
	pop     es
	pop     edi
	leave
	ret
_FillScreenBuffer ENDP

LOWLEVEL_TEXT ENDS

	END
