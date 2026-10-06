; Serpent Isle SI.EXE, resident segment 156 (file offsets 0x03f2a6 to 0x03f2b2, 12 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

	.MODEL  MEDIUM
	.386

	PUBLIC  _IsFlatModeBlocked, _EnterFlatMode, _LeaveFlatMode

	.CODE

; Three entry points that report nothing: each returns 0 in eax (and so in dx:ax).
_IsFlatModeBlocked PROC FAR
	xor     eax, eax
	ret
_IsFlatModeBlocked ENDP

_EnterFlatMode PROC FAR
	xor     eax, eax
	ret
_EnterFlatMode ENDP

_LeaveFlatMode PROC FAR
	xor     eax, eax
	ret
_LeaveFlatMode ENDP

	END
