; Black Gate U7.EXE, resident segment 182 (file offsets 0x03fda4 to 0x040121, 893 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.
; Folder inferred from compiler flags and link order.

	.MODEL  MEDIUM
	.386
	LOCALS

	PUBLIC  _XMSLargestKilobytes, _XMSHandle, _XMSBlockAddress
	PUBLIC  _FrameLinearAddress, _UnhookInt15, _InstallVector
	PUBLIC  _ClaimExtendedMemory, _FindXMSDriver, _QueryXMSFree
	PUBLIC  _AllocateXMS, _LockXMS, _UnlockXMS, _FreeXMS, _EnableA20Local
	PUBLIC  _EnableA20Global, _DisableA20Local, _DisableA20Global
	PUBLIC  _RequestHMA, _ReleaseHMA, _RequestUMB, _ReleaseUMB

	EXTRN   _EnterFlatMode:FAR

	.DATA
	EXTRN   _FlatModeFlags:WORD

_XMSDriverEntry         dd  0           ; XMS driver entry point
_XMSFreeKilobytes       dw  0           ; free extended memory, KB
_XMSLargestKilobytes    dw  0           ; largest free block, KB
_XMSHandle              dw  0           ; our XMS handle
_XMSBlockAddress        dd  0           ; linear address of our block

_UMBSegment     dw  0                   ; upper memory block segment
_UMBParagraphs  dw  0                   ; and its size in paragraphs

_ExtendedKilobytes  dw  0               ; extended memory reported by INT 15h, KB
_ExtendedMemoryEnd  dd  0               ; linear end of extended memory
vdiskSignature      db  'VDISK', 0

	.CODE

oldInt15    dd  0                       ; the INT 15h handler ours displaced

; The caller's first argument, a far pointer, as a linear address in dx:ax.
; It has no frame of its own and reads the pointer through the caller's.
_FrameLinearAddress PROC FAR
	mov     eax, [bp+6]
	push    eax
	xor     eax, eax
	xor     ebx, ebx
	pop     bx
	pop     ax
	shl     eax, 4
	add     eax, ebx
	push    eax
	xor     eax, eax
	pop     ax
	pop     dx
	ret
_FrameLinearAddress ENDP

; Once we own extended memory, INT 15h function 88h reports none left.
Int15Handler PROC FAR
	cli
	cmp     ah, 88h
	jne     @@chain
	xor     eax, eax
	iret
@@chain:
	jmp     cs:oldInt15
Int15Handler ENDP

; Put back the INT 15h handler ours displaced.
_UnhookInt15 PROC FAR
	push    bp
	mov     bp, sp
	push    si
	push    di
	push    ds
	push    bx
	push    dx
	push    es
	mov     si, 15h * 4
	xor     ax, ax
	mov     ds, ax
	mov     eax, cs:oldInt15
	mov     [si], eax
	xor     ax, ax
	pop     es
	pop     dx
	pop     bx
	pop     ds
	pop     di
	pop     si
	pop     bp
	ret
_UnhookInt15 ENDP

; Install handler as interrupt n, keeping the previous one in *old.
_InstallVector PROC FAR
	ARG     n:WORD, handler:DWORD, old:DWORD
	push    bp
	mov     bp, sp
	push    eax
	push    esi
	push    ds
	push    es
	xor     eax, eax
	mov     ax, n
	shl     eax, 2
	mov     esi, eax
	xor     eax, eax
	mov     es, ax
	mov     eax, es:[si]
	push    eax
	mov     eax, handler
	cli
	mov     es:[si], eax
	mov     eax, old
	push    eax
	pop     si
	pop     es
	pop     eax
	mov     es:[si], eax
	sti
	pop     es
	pop     ds
	pop     esi
	pop     eax
	pop     bp
	ret
_InstallVector ENDP

; Take all extended memory above what a VDISK RAM disk already uses,
; hooking INT 15h so nobody else finds any. Returns the linear address
; of the first free byte, or 0.
_ClaimExtendedMemory PROC FAR
	push    bp
	mov     bp, sp
	push    ebx
	push    ecx
	push    esi
	push    edi
	push    ds
	push    es
	mov     ax, _FlatModeFlags
	and     ax, 1
	je      @@sized
	call    _EnterFlatMode
@@sized:
	xor     eax, eax
	mov     ah, 88h
	int     15h
	mov     _ExtendedKilobytes, ax
	mov     _XMSLargestKilobytes, ax
	shl     eax, 10
	add     eax, 100000h
	mov     _ExtendedMemoryEnd, eax
	or      eax, eax
	je      @@none
	mov     si, 19h * 4                 ; a VDISK hooks INT 19h and signs its handler
	mov     dx, ds
	xor     eax, eax
	xor     ebx, ebx
	mov     ds, ax
	lodsw
	mov     bx, ax
	lodsw
	shl     eax, 4
	add     eax, ebx
	mov     esi, eax
	push    esi
	mov     ecx, 64
@@scan:
	mov     al, [esi]
	cmp     al, 'V'
	je      @@maybe
@@next:
	inc     esi
	loop    @@scan
	pop     esi
	jmp     @@noVdisk
@@maybe:
	push    eax
	push    ecx
	push    esi
	mov     di, offset vdiskSignature
	mov     ax, ss
	mov     es, ax
	mov     cx, 5
	pushf
	cld
	repe    cmpsb
	popf
	or      cx, cx
	je      @@vdisk
	pop     esi
	pop     ecx
	pop     eax
	jmp     @@next
@@vdisk:
	pop     eax
	pop     eax
	pop     eax
	pop     esi
	add     esi, 44                     ; the 24-bit address of its first free byte
	xor     eax, eax
	xor     ebx, ebx
	mov     ax, [esi]
	inc     esi
	inc     esi
	mov     bx, ax
	xor     ax, ax
	mov     al, [esi]
	inc     esi
	push    ax
	xor     eax, eax
	push    ax
	pop     eax
	or      eax, ebx
	mov     ss:_XMSBlockAddress, eax
	mov     esi, 100000h
	add     esi, 30                     ; the KB its boot record says it uses
	mov     ax, [esi]
	shl     eax, 10
	mov     ebx, ss:_XMSBlockAddress
	cmp     eax, ebx
	je      @@hook
	cmp     eax, ebx
	jae     @@start
	mov     eax, ebx
@@start:
	mov     ss:_XMSBlockAddress, eax
@@hook:
	mov     si, 15h * 4
	xor     ax, ax
	mov     ds, ax
	mov     di, offset Int15Handler
	mov     eax, [si]
	mov     cs:oldInt15, eax
	mov     [si+2], cs
	mov     [si], di
	mov     ah, 88h
	int     15h
	or      ax, ax
	xor     edx, edx
	mov     eax, ss:_XMSBlockAddress
	push    eax
	xor     eax, eax
	pop     ax
	pop     dx
	jmp     @@done
@@noVdisk:
	mov     eax, 100000h
	jmp     @@start
@@none:
	xor     eax, eax
	xor     edx, edx
@@done:
	pop     es
	pop     ds
	pop     edi
	pop     esi
	pop     ecx
	pop     ebx
	pop     bp
	ret
_ClaimExtendedMemory ENDP

; Ask DOS for the XMS driver's entry point.
_FindXMSDriver PROC FAR
	push    bp
	mov     bp, sp
	push    si
	push    di
	push    ds
	push    bx
	push    dx
	push    es
	mov     ax, 4310h
	int     2Fh
	mov     word ptr _XMSDriverEntry, bx
	mov     word ptr _XMSDriverEntry+2, es
	mov     ax, 1
	pop     es
	pop     dx
	pop     bx
	pop     ds
	pop     di
	pop     si
	pop     bp
	ret
_FindXMSDriver ENDP

; The free extended memory, in KB.
_QueryXMSFree PROC FAR
	push    bp
	mov     bp, sp
	push    si
	push    di
	push    ds
	push    bx
	push    dx
	push    es
	mov     ah, 8
	call    _XMSDriverEntry
	or      ax, ax
	je      @@failed
	mov     _XMSFreeKilobytes, dx
	mov     _XMSLargestKilobytes, ax
@@failed:
	pop     es
	pop     dx
	pop     bx
	pop     ds
	pop     di
	pop     si
	pop     bp
	ret
_QueryXMSFree ENDP

; Allocate kb KB of extended memory. Returns the new handle, or 0.
_AllocateXMS PROC FAR
	ARG     kb:WORD
	push    bp
	mov     bp, sp
	push    si
	push    di
	push    ds
	push    bx
	push    dx
	push    es
	mov     ah, 9
	mov     dx, kb
	call    _XMSDriverEntry
	or      ax, ax
	je      @@failed
	mov     _XMSHandle, dx
	mov     ax, dx
@@failed:
	pop     es
	pop     dx
	pop     bx
	pop     ds
	pop     di
	pop     si
	pop     bp
	ret
_AllocateXMS ENDP

; Pin our block and note its linear address.
_LockXMS PROC FAR
	push    bp
	mov     bp, sp
	push    si
	push    di
	push    ds
	push    bx
	push    dx
	push    es
	mov     ah, 0Ch
	mov     dx, _XMSHandle
	call    _XMSDriverEntry
	or      ax, ax
	je      @@failed
	mov     word ptr _XMSBlockAddress, bx
	mov     word ptr _XMSBlockAddress+2, dx
@@failed:
	pop     es
	pop     dx
	pop     bx
	pop     ds
	pop     di
	pop     si
	pop     bp
	ret
_LockXMS ENDP

; Unpin our block.
_UnlockXMS PROC FAR
	push    bp
	mov     bp, sp
	push    si
	push    di
	push    ds
	push    bx
	push    dx
	push    es
	mov     ah, 0Dh
	mov     dx, _XMSHandle
	call    _XMSDriverEntry
	pop     es
	pop     dx
	pop     bx
	pop     ds
	pop     di
	pop     si
	pop     bp
	ret
_UnlockXMS ENDP

; Free our block.
_FreeXMS PROC FAR
	push    bp
	mov     bp, sp
	push    si
	push    di
	push    ds
	push    bx
	push    dx
	push    es
	mov     ah, 0Ah
	mov     dx, _XMSHandle
	call    _XMSDriverEntry
	pop     es
	pop     dx
	pop     bx
	pop     ds
	pop     di
	pop     si
	pop     bp
	ret
_FreeXMS ENDP

; The XMS driver's A20 switches.
_EnableA20Local PROC FAR
	push    bp
	mov     bp, sp
	push    si
	push    di
	push    ds
	push    bx
	push    dx
	push    es
	mov     ah, 5
	call    _XMSDriverEntry
	pop     es
	pop     dx
	pop     bx
	pop     ds
	pop     di
	pop     si
	pop     bp
	ret
_EnableA20Local ENDP

_EnableA20Global PROC FAR
	push    bp
	mov     bp, sp
	push    si
	push    di
	push    ds
	push    bx
	push    dx
	push    es
	mov     ah, 3
	call    _XMSDriverEntry
	pop     es
	pop     dx
	pop     bx
	pop     ds
	pop     di
	pop     si
	pop     bp
	ret
_EnableA20Global ENDP

_DisableA20Local PROC FAR
	push    bp
	mov     bp, sp
	push    si
	push    di
	push    ds
	push    bx
	push    dx
	push    es
	mov     ah, 6
	call    _XMSDriverEntry
	pop     es
	pop     dx
	pop     bx
	pop     ds
	pop     di
	pop     si
	pop     bp
	ret
_DisableA20Local ENDP

_DisableA20Global PROC FAR
	push    bp
	mov     bp, sp
	push    si
	push    di
	push    ds
	push    bx
	push    dx
	push    es
	mov     ah, 4
	call    _XMSDriverEntry
	pop     es
	pop     dx
	pop     bx
	pop     ds
	pop     di
	pop     si
	pop     bp
	ret
_DisableA20Global ENDP

; Claim the high memory area.
_RequestHMA PROC FAR
	push    bp
	mov     bp, sp
	push    si
	push    di
	push    ds
	push    bx
	push    dx
	push    es
	mov     ah, 1
	mov     dx, 0FFFFh
	call    _XMSDriverEntry
	pop     es
	pop     dx
	pop     bx
	pop     ds
	pop     di
	pop     si
	pop     bp
	ret
_RequestHMA ENDP

; Give back the high memory area.
_ReleaseHMA PROC FAR
	push    bp
	mov     bp, sp
	push    si
	push    di
	push    ds
	push    bx
	push    dx
	push    es
	mov     ah, 2
	call    _XMSDriverEntry
	pop     es
	pop     dx
	pop     bx
	pop     ds
	pop     di
	pop     si
	pop     bp
	ret
_ReleaseHMA ENDP

; Claim the largest upper memory block, up to 384 KB.
_RequestUMB PROC FAR
	push    bp
	mov     bp, sp
	push    si
	push    di
	push    ds
	push    bx
	push    dx
	push    es
	mov     dx, 384 * 64                ; 384 KB in paragraphs
@@again:
	mov     ah, 10h
	call    _XMSDriverEntry
	or      ax, ax
	jne     @@got
	or      dx, dx
	je      @@none
	cmp     bl, 0B0h                    ; only a smaller block: ask for that
	je      @@again
@@none:
	xor     ax, ax
	mov     dx, ax
	mov     bx, ax
@@got:
	mov     _UMBSegment, bx
	mov     _UMBParagraphs, dx
	pop     es
	pop     dx
	pop     bx
	pop     ds
	pop     di
	pop     si
	pop     bp
	ret
_RequestUMB ENDP

; Give back our upper memory block.
_ReleaseUMB PROC FAR
	push    bp
	mov     bp, sp
	push    si
	push    di
	push    ds
	push    bx
	push    dx
	push    es
	mov     ah, 11h
	mov     dx, _UMBSegment
	call    _XMSDriverEntry
	pop     es
	pop     dx
	pop     bx
	pop     ds
	pop     di
	pop     si
	pop     bp
	ret
_ReleaseUMB ENDP

	.DATA
int15Vector dd  Int15Handler            ; nothing reads it

	END
