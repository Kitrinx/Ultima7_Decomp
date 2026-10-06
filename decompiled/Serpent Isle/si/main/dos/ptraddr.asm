; Serpent Isle SI.EXE, resident segment 121 (file offsets 0x03d584 to 0x03d5a5, 33 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

	.MODEL  MEDIUM, PASCAL

	PUBLIC  POINTERTOLINEAR

	.CODE

; The linear address of p, in dx:ax.
POINTERTOLINEAR PROC FAR p:DWORD
	mov     ax, word ptr p+2
	xor     dx, dx
	shl     ax, 1                       ; dl:ax = segment * 16
	rcl     dl, 1
	shl     ax, 1
	rcl     dl, 1
	shl     ax, 1
	rcl     dl, 1
	shl     ax, 1
	rcl     dl, 1
	add     ax, word ptr p
	adc     dl, dh
	ret
POINTERTOLINEAR ENDP

	END
