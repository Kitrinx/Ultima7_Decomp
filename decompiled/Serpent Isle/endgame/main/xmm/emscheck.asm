; Serpent Isle ENDGAME.EXE, resident segment 81 (file offsets 0x014ab8 to 0x014aec, 52 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

	.MODEL  MEDIUM
	LOCALS

	PUBLIC  _DetectEmsDriver

	.DATA
emsDeviceName   db  'EMMXXXX0', 0

	.CODE

; Returns 1 when an expanded memory manager is present: its device name opens as a character
; device. 0 otherwise.
_DetectEmsDriver PROC FAR
	push    bx
	push    dx
	push    ds
	mov     dx, offset emsDeviceName
	mov     ax, 3D00h
	int     21h
	jc      @@absent
	mov     bx, ax
	mov     ax, 4400h
	int     21h
	jc      @@close
	and     dx, 80h
	jz      @@close
	mov     ah, 3Eh
	int     21h
	jc      @@close
	mov     ax, 1
	jmp     @@done
@@close:
	mov     ah, 3Eh
	int     21h
	jc      @@close
@@absent:
	xor     ax, ax
@@done:
	pop     ds
	pop     dx
	pop     bx
	ret
_DetectEmsDriver ENDP

	END
