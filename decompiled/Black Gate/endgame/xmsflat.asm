; Black Gate ENDGAME.EXE, resident segment 84 (file offsets 0x015366 to 0x0153c9, 99 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

	.MODEL  MEDIUM
	.386

	EXTRN   InitFlatMode:FAR, SetFlatMode:FAR, ClearFlatMode:FAR

	PUBLIC  _IsFlatModeBlocked, _EnterFlatMode, _LeaveFlatMode

	.CODE

; C entry points to the flat mode switches. Every register is kept, so the
; answer crosses popad in the carry: shr moves bit 0 of ax there and rcl
; brings it back into a cleared eax as 0 or 1.
FLATCALL MACRO target
	pushfd
	pushad
	push    ds
	push    es
	push    fs
	push    gs
	call    target
	shr     ax, 1
	pop     gs
	pop     fs
	pop     es
	pop     ds
	popad
	xor     eax, eax
	rcl     ax, 1
	popfd
	ret
ENDM

_IsFlatModeBlocked PROC FAR
	FLATCALL InitFlatMode
_IsFlatModeBlocked ENDP

_EnterFlatMode PROC FAR
	FLATCALL SetFlatMode
_EnterFlatMode ENDP

_LeaveFlatMode PROC FAR
	FLATCALL ClearFlatMode
_LeaveFlatMode ENDP

	END
