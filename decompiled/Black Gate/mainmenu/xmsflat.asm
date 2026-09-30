; Black Gate MAINMENU.EXE, resident segment 3 (file offsets 0x00b5a6 to 0x00b609, 99 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

	.MODEL  MEDIUM
	.386

	EXTRN   InitFlatMode:FAR, SetFlatMode:FAR, ClearFlatMode:FAR

	PUBLIC  _IsFlatModeBlocked, _EnterFlatMode, _LeaveFlatMode

	.CODE

; C entry points to the flat mode switches. Every register is kept, so the
; answer crosses popad in the carry: shl moves bit 15 of ax there and rcl
; brings it back as 0 or 1.
FLATCALL MACRO target
	pushfd
	pushad
	push    ds
	push    es
	push    fs
	push    gs
	call    target
	shl     ax, 1
	pop     gs
	pop     fs
	pop     es
	pop     ds
	popad
	rcl     ax, 1
	and     ax, 1
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
