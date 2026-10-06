; Serpent Isle INTRO.EXE, resident segment 35 (file offsets 0x00e37c to 0x00e487, 267 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

	.MODEL  MEDIUM, C
	LOCALS

EMS_INT         EQU     67h
EMS_MAP         EQU     44h
EMS_SAVE_MAP    EQU     47h
EMS_RESTORE_MAP EQU     48h
EMS_TAG         EQU     0C0h            ; top bits of an EMS pointer's segment
FRAME_PAGES     EQU     4

	PUBLIC  IsEmsPointer, MapEmsPointer, SaveEmsMap, RestoreEmsMap
	EXTRN   EmsFrame:WORD, EmsPages:WORD, EmsPointer:DWORD, EmsHandle:WORD
	EXTRN   EmsActive:WORD, SavedEmsPointer:DWORD

	.CODE

; Returns 1 when p is an EMS pointer.
IsEmsPointer        PROC FAR p:DWORD
	mov     dx, word ptr p+2
	mov     ax, word ptr p
	mov     bh, dh
	and     bh, EMS_TAG
	cmp     bh, EMS_TAG
	jne     @@no
	mov     ax, 1
	ret
@@no:
	xor     ax, ax
	ret
IsEmsPointer        ENDP

; Returns p itself unless it is an EMS pointer; then maps its pages into the
; frame and returns where it lies there, or 0 when that fails.
MapEmsPointer       PROC FAR p:DWORD
	LOCAL   slot:BYTE, page:WORD
	push    si
	push    di
	mov     dx, word ptr p+2
	mov     ax, word ptr p
	mov     bh, dh
	and     bh, EMS_TAG
	cmp     bh, EMS_TAG
	jne     @@done
	cmp     EmsActive, 0
	je      @@fail
	cmp     dx, word ptr EmsPointer+2
	jne     @@map
	cmp     ax, word ptr EmsPointer
	je      @@mapped
@@map:
	mov     word ptr EmsPointer, ax
	mov     word ptr EmsPointer+2, dx
	and     dh, NOT EMS_TAG
	cmp     dx, EmsPages
	jae     @@fail
	mov     page, dx
	mov     slot, 0
@@next:
	mov     ah, EMS_MAP
	mov     al, slot
	mov     bx, page
	cmp     bx, EmsPages
	jae     @@mapped
	mov     dx, EmsHandle
	int     EMS_INT
	or      ah, ah
	jne     @@fail
	inc     slot
	inc     page
	cmp     slot, FRAME_PAGES
	jb      @@next
@@mapped:
	mov     dx, EmsFrame
	mov     ax, word ptr p
@@done:
	pop     di
	pop     si
	ret
@@fail:
	xor     ax, ax
	xor     dx, dx
	mov     word ptr EmsPointer, ax
	mov     word ptr EmsPointer+2, ax
	pop     di
	pop     si
	ret
MapEmsPointer       ENDP

; Save the driver's page map and the pointer it holds. Returns 1 on success.
SaveEmsMap          PROC FAR
	push    si
	push    di
	push    ds
	push    bp
	cmp     EmsActive, 0
	je      @@fail
	mov     ah, EMS_SAVE_MAP
	mov     dx, EmsHandle
	int     EMS_INT
	or      ah, ah
	jne     @@fail
	mov     ax, 1
	mov     bx, word ptr EmsPointer
	mov     word ptr SavedEmsPointer, bx
	mov     bx, word ptr EmsPointer+2
	mov     word ptr SavedEmsPointer+2, bx
	pop     bp
	pop     ds
	pop     di
	pop     si
	ret
@@fail:
	xor     ax, ax
	pop     bp
	pop     ds
	pop     di
	pop     si
	ret
SaveEmsMap          ENDP

; Restore the page map SaveEmsMap saved. Returns 1 on success.
RestoreEmsMap       PROC FAR
	push    si
	push    di
	push    ds
	push    bp
	cmp     EmsActive, 0
	je      @@fail
	mov     ah, EMS_RESTORE_MAP
	mov     dx, EmsHandle
	int     EMS_INT
	or      ah, ah
	jne     @@fail
	mov     ax, 1
	mov     bx, word ptr SavedEmsPointer
	mov     word ptr EmsPointer, bx
	mov     bx, word ptr SavedEmsPointer+2
	mov     word ptr EmsPointer+2, bx
	pop     bp
	pop     ds
	pop     di
	pop     si
	ret
@@fail:
	xor     ax, ax
	pop     bp
	pop     ds
	pop     di
	pop     si
	ret
RestoreEmsMap       ENDP

	END
