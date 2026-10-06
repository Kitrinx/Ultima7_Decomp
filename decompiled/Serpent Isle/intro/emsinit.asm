; Serpent Isle INTRO.EXE, resident segment 34 (file offsets 0x00e254 to 0x00e37b, 295 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

	.MODEL  MEDIUM, C
	LOCALS

EMS_INT         EQU     67h
EMS_STATUS      EQU     40h
EMS_FRAME       EQU     41h
EMS_PAGE_COUNT  EQU     42h
EMS_ALLOCATE    EQU     43h
EMS_MAP         EQU     44h
EMS_FREE        EQU     45h
EMS_VERSION     EQU     46h
DEVICE_NAME     EQU     0Ah             ; offset of the name in a device driver header

	PUBLIC  EmsVersion, EmsFrame, EmsTotalPages, EmsPages, EmsPointer
	PUBLIC  EmsHandle, EmsReserve, EmsReserveHandle, EmsActive, SavedEmsPointer
	PUBLIC  OpenEms, CloseEms
	EXTRN   MapEmsPointer:FAR

	.DATA

emsName         db      'EMMXXXX0', 0
EmsVersion      dw      0
EmsFrame        dw      0               ; segment of the page frame
EmsTotalPages   dw      0
EmsPages        dw      0               ; pages we hold
EmsPointer      dd      0               ; the EMS pointer now mapped in
EmsHandle       dw      0
EmsReserve      dw      0               ; pages to keep out of our hands
EmsReserveHandle dw     0
unused          db      0
EmsActive       dw      0
SavedEmsPointer dd      0

	.CODE

; Take expanded memory when a 3.0 or later driver is there, first allocating
; EmsReserve pages so they stay free for others. Returns 1 when we hold pages.
OpenEms             PROC FAR
	push    si
	push    di
	mov     ax, 3500h + EMS_INT
	int     21h
	mov     di, DEVICE_NAME
	mov     si, offset emsName
	mov     cx, 8
	cld
	repe    cmpsb
	jne     @@fail
	mov     ah, EMS_STATUS
	int     EMS_INT
	or      ah, ah
	jne     @@fail
	mov     ah, EMS_VERSION
	int     EMS_INT
	xor     ah, ah
	cmp     al, 30h
	mov     EmsVersion, ax
	jb      @@fail
	mov     ah, EMS_FRAME
	int     EMS_INT
	or      ah, ah
	jne     @@fail
	mov     EmsFrame, bx
	mov     bx, EmsReserve
	cmp     bx, 0
	je      @@reserved
	mov     ah, EMS_ALLOCATE
	int     EMS_INT
	or      ah, ah
	jne     @@fail
	mov     EmsReserveHandle, dx
@@reserved:
	mov     ah, EMS_PAGE_COUNT
	int     EMS_INT
	or      ah, ah
	jne     @@release
	mov     EmsTotalPages, dx
	mov     EmsPages, bx
	or      bx, bx
	je      @@release
	mov     ah, EMS_ALLOCATE
	int     EMS_INT
	or      ah, ah
	jne     @@release
	mov     EmsHandle, dx
	mov     EmsActive, 1
	call    far ptr WriteEmsHeader
	or      ah, ah
	jne     @@release
	mov     ax, 1
	jmp     short @@done
@@release:
	cmp     EmsReserve, 0
	je      @@fail
	mov     ah, EMS_FREE
	mov     dx, EmsReserveHandle
	int     EMS_INT
@@fail:
	mov     EmsReserve, 0
	mov     EmsReserveHandle, 0
	mov     ax, 0
@@done:
	mov     EmsActive, ax
	pop     di
	pop     si
	ret
OpenEms             ENDP

; Write the usable size, all our pages less this 4-byte header, at the start of
; the first page. Leaves AH nonzero on failure.
WriteEmsHeader      PROC FAR
	LOCAL   saved:DWORD
	mov     ah, 1
	cmp     EmsActive, 0
	je      @@done
	les     ax, EmsPointer
	mov     word ptr saved, ax
	mov     word ptr saved+2, es
	mov     ax, EMS_MAP * 256       ; logical page 0 into physical page 0
	mov     dx, EmsHandle
	xor     bx, bx
	int     EMS_INT
	or      ah, ah
	jne     @@restore
	mov     dx, EmsPages            ; times 16K
	shr     dx, 1
	rcr     ax, 1
	shr     dx, 1
	rcr     ax, 1
	sub     ax, 4
	sbb     dx, 0
	mov     es, EmsFrame
	mov     es:[0], ax
	mov     es:[2], dx
@@restore:
	push    word ptr saved+2
	push    word ptr saved
	call    MapEmsPointer
	add     sp, 4
@@done:
	ret
WriteEmsHeader      ENDP

; Give back our pages and the reserved ones.
CloseEms            PROC FAR
	cmp     EmsActive, 0
	je      @@done
	mov     EmsActive, 0
	mov     ah, EMS_FREE
	mov     dx, EmsHandle
	int     EMS_INT
	cmp     EmsReserve, 0
	je      @@done
	mov     EmsReserve, 0
	mov     ah, EMS_FREE
	mov     dx, EmsReserveHandle
	int     EMS_INT
@@done:
	ret
CloseEms            ENDP

	END
