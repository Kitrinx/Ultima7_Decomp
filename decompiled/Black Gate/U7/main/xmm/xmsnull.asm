; Black Gate U7.EXE, resident segment 157 (file offsets 0x03f6f4 to 0x03f700, 12 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.
; Folder inferred from compiler flags and link order.

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
