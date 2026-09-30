; Black Gate U7.EXE, routines in resident segment 191 (file offsets 0x04084f to 0x04112c, 2269 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.
; Public and external names are Borland's own; the internal names are descriptive.

; The Borland C++ overlay manager: OVERLAY.LIB's OVRMAN. The linker joins it
; with the library's OVRUSER and OVRDATA into one _OVRTEXT_ segment. This OVRMAN
; differs from the one in Borland C++ 2.0 to 3.1: it opens the executable with
; AX = 3D00h, where they open it with a mode byte set to 20h (deny write)
; under DOS 3 and later.
; NOSMART keeps a far call or lea as written where TASM would rewrite it.

_TEXT   SEGMENT BYTE PUBLIC 'CODE'
_TEXT   ENDS
_DATA   SEGMENT WORD PUBLIC 'DATA'
	EXTRN   __OvrSize:WORD
	EXTRN   __envseg:WORD
	EXTRN   __psp:WORD
_DATA   ENDS
_OVRTEXT_   SEGMENT BYTE PUBLIC 'CODE'
_OVRTEXT_   ENDS
_OVRDATA_   SEGMENT PARA PUBLIC 'OVRINFO'
	EXTRN   __CALLEMSEXIT:WORD
	EXTRN   __CALLEXTEMSSWAP:WORD
	EXTRN   __CALLEXTEXIT:WORD
	EXTRN   __OVRFILENAME:WORD
	EXTRN   __OVRHOOK__:DWORD
			dw  0
TrapVector  dd  TrapHandler             ; the trap interrupt's other vector
OpenMode    db  0
_OVRDATA_   ENDS
_EXEINFO_   SEGMENT PARA PUBLIC 'OVRINFO'
	EXTRN   __EXENAME__:BYTE
	EXTRN   __SEGTABEND__:BYTE
	EXTRN   __SEGTABLE__:WORD
_EXEINFO_   ENDS
_STUB_  SEGMENT PARA PUBLIC 'OVRINFO'
	EXTRN   __OVRTRAP__:WORD
	EXTRN   __OvrCodeList:WORD
	EXTRN   __OvrDosHandle:WORD
	EXTRN   __OvrFileBase:WORD
	EXTRN   __OvrHeapEnd:WORD
	EXTRN   __OvrHeapOrg:WORD
	EXTRN   __OvrHeapPtr:WORD
	EXTRN   __OvrLoadCount:WORD
	EXTRN   __OvrLoadList:WORD
	EXTRN   __OvrMinHeapSize:WORD
	EXTRN   __OvrRetrySize:WORD
	EXTRN   __OvrTrapCount:WORD
_STUB_  ENDS
_EXTSEG_    SEGMENT PARA PUBLIC 'OVRINFO'
_EXTSEG_    ENDS
_EMSSEG_    SEGMENT PARA PUBLIC 'OVRINFO'
_EMSSEG_    ENDS
_VDISKSEG_  SEGMENT PARA PUBLIC 'OVRINFO'
_VDISKSEG_  ENDS
DGROUP  GROUP   _DATA
_OVRGROUP_  GROUP   _VDISKSEG_, _EMSSEG_, _EXTSEG_, _STUB_, _OVRDATA_, _EXEINFO_

	EXTRN   __OverlayHalt:FAR

	LOCALS

; What __OVRINIT leaves in CX when it fails; it returns -2 when the
; file is not found and -1 otherwise.
ERR_FAIL        EQU -1
ERR_NOTFOUND    EQU -2
ERR_READ        EQU -3
ERR_FORMAT      EQU -4
ERR_NOMEMORY    EQU -5

OP_INT          EQU 0CDh                ; a jump entry still traps
OP_JMPFAR       EQU 0EAh                ; a jump entry reaches loaded code

; The resident stub in front of each overlaid segment's jump table.
OVRSTUB STRUC
stub_trap       dw  ?                   ; int 3Fh
stub_retofs     dw  ?
stub_filepos    dd  ?                   ; where the code lies in the executable
stub_codesize   dw  ?
stub_fixups     dw  ?                   ; bytes of relocations after the code
stub_entries    dw  ?                   ; entries in the jump table
				dw  ?
stub_loadseg    dw  ?                   ; the code's segment while loaded, else 0
stub_next       dw  ?                   ; next stub in the code list
				dw  ?, ?
stub_reader     dw  ?                   ; the routine that reads the code in
stub_flags      db  ?
stub_usage      db  ?
stub_nextload   dw  ?                   ; next stub in the load list
				dw  ?
stub_jumps      db  ?                   ; int 3Fh while unloaded, jmp far once loaded
OVRSTUB ENDS

; A jump table entry while its overlay is out, and once it is in.
TRAPJUMP    STRUC
tj_int      dw  ?                       ; int 3Fh
tj_target   dw  ?                       ; the routine's offset in the overlay
			db  ?
TRAPJUMP    ENDS
FARJUMP STRUC
fj_opcode   db  ?                       ; jmp far
fj_offset   dw  ?
fj_segment  dw  ?
FARJUMP ENDS

; The paragraph in front of a loaded overlay's code.
OVRBLOCK    STRUC
			db  14 dup (?)
blk_stub    dw  ?
OVRBLOCK    ENDS

; An entry of the linker's segment table.
SEGENTRY    STRUC
seg_base    dw  ?                       ; for an overlaid segment, its stub
seg_size    dw  ?
seg_flags   dw  ?                       ; 2 = overlaid
			dw  ?
SEGENTRY    ENDS

; A frame on the stack: saved BP, then the return address. The trap
; interrupt's frame has the flags above that.
STACKFRAME  STRUC
sf_bp       dw  ?
sf_ip       dw  ?
sf_cs       dw  ?
sf_flags    dw  ?
STACKFRAME  ENDS

; The start of an MZ header, read into __OVRINIT's frame.
EXEHEADER   STRUC
exe_magic       dw  ?                   ; 'MZ'
exe_lastpage    dw  ?                   ; bytes used in the last page
exe_pages       dw  ?                   ; 512-byte pages, the last one partial
				dw  7 dup (?)
EXEHEADER   ENDS

; The header of each block appended after the image. The overlays
; are in the one marked FBOV.
FBOVHEADER  STRUC
fbov_magic  dd  ?                       ; 'FBOV'
fbov_size   dd  ?                       ; bytes that follow this header
			dd  ?, ?
FBOVHEADER  ENDS

_OVRTEXT_   SEGMENT
	ASSUME  CS:_OVRTEXT_, DS:_OVRGROUP_

	PUBLIC  __ISOVERLAYMOD
	PUBLIC  __CHECKOVERLAY
	PUBLIC  __OVRINIT
	PUBLIC  __OVRGROUP
	PUBLIC  __InitModules
	PUBLIC  __DGROUP
	PUBLIC  __CHECKSTACK
	PUBLIC  __OVREXIT
	PUBLIC  __ReadOvrDisk

__OVRGROUP  dw  _OVRGROUP_
__DGROUP    dw  DGROUP
PathName    db  'PATH='
LoadSeg     dw  0                       ; the program's first paragraph

; Opens the executable named by fileName, or the program itself, checks its
; MZ header, finds the FBOV overlay data after the image, sizes the overlay
; heap and hooks the trap interrupt. Returns 0, or a negative error code.
__OVRINIT   PROC FAR
	ARG     fileName:DWORD, heapEnd:WORD, heapStart:WORD
	LOCAL   header:EXEHEADER = initFrame
	push    bp
	mov     bp, sp
	sub     sp, initFrame
	push    ds
	mov     ds, cs:__OVRGROUP
	push    si
	push    di
	cld
	cmp     word ptr __OVRTRAP__, 0
	jne     @@hasOverlays
	jmp     @@ok                        ; nothing is overlaid

	; With no name given, the program's own path is tried first.
@@hasOverlays:
	call    OpenFromExeDir
	jae     @@opened
	mov     ax, word ptr fileName
	or      ax, word ptr fileName+2
	jne     @@named
	mov     word ptr fileName, offset __EXENAME__
	mov     word ptr fileName+2, seg __EXENAME__
@@named:
	call    OpenInCurrentDir
	jae     @@opened
	call    OpenFromPath
	jae     @@opened
	mov     ax, ERR_NOTFOUND
	jmp     @@return
@@closeFail:
	mov     ah, 3Eh                     ; close
	int     21h
	mov     ax, ERR_FAIL
	jmp     @@return

@@opened:
	mov     bx, ax
	mov     __OvrDosHandle, bx
	mov     cx, SIZE EXEHEADER
	call    ReadHeader
	mov     cx, ERR_READ
	jb      @@closeFail
	xor     ax, ax
	xor     dx, dx
	cmp     header.exe_magic, 5A4Dh     ; 'MZ'
	je      @@isExe
	mov     cx, ERR_FORMAT
	jmp     @@closeFail

	; The image ends at pages * 512, less the unused part of the last
	; page, rounded up to a paragraph.
@@isExe:
	mov     ax, header.exe_pages
	mov     cx, header.exe_lastpage
	jcxz    @@imageSize
	dec     ax
@@imageSize:
	mov     dx, 512
	mul     dx
	add     ax, cx
	add     ax, 0Fh
	adc     dx, 0
	and     ax, 0FFF0h

	; Walk the blocks appended after the image until FBOV.
@@nextBlock:
	push    dx
	push    ax
	mov     cx, dx
	mov     dx, ax
	mov     ax, 4200h                   ; seek from the start
	int     21h
	mov     cx, SIZE FBOVHEADER
	call    ReadHeader
	pop     ax
	pop     dx
	mov     cx, ERR_READ
	jb      @@closeFail
	add     ax, SIZE FBOVHEADER
	adc     dx, 0
	cmp     word ptr header.fbov_magic, 4246h ; 'FB'
	mov     cx, ERR_FORMAT
	je      @@isFB
	jmp     @@closeFail
@@isFB:
	cmp     word ptr header.fbov_magic+2, 564Fh ; 'OV'
	je      @@found
	add     ax, word ptr header.fbov_size
	adc     dx, word ptr header.fbov_size+2
	jmp     @@nextBlock

@@found:
	mov     __OvrFileBase, ax
	mov     [__OvrFileBase+2], dx
	mov     es, cs:__DGROUP
	mov     ax, word ptr header.fbov_size
	mov     es:__OvrSize, ax
	mov     ax, word ptr header.fbov_size+2
	mov     es:[__OvrSize+2], ax
	mov     ah, 3Eh                     ; close
	int     21h
	mov     word ptr __OvrDosHandle, 0
	NOSMART
	call    far ptr SwapTrap
	SMART

	; The heap's first paragraph is left for the block header.
	mov     ax, heapStart
	inc     ax
	mov     __OvrHeapOrg, ax
	mov     __OvrHeapPtr, ax
	mov     bx, heapEnd
	mov     __OvrHeapEnd, bx
	call    __InitModules
	mov     bx, __OvrHeapEnd
	sub     bx, __OvrHeapOrg
	cmp     bx, __OvrMinHeapSize
	jae     @@heapOk
	mov     cx, ERR_NOMEMORY
	jmp     @@closeFail
@@heapOk:
	shr     bx, 1
	shr     bx, 1
	mov     __OvrRetrySize, bx
	call    LoadAtStart
	jae     @@ok
	mov     bx, __OvrDosHandle
	mov     cx, ERR_FAIL
	jmp     @@return
@@ok:
	xor     ax, ax
@@return:
	pop     di
	pop     si
	pop     ds
	mov     sp, bp
	pop     bp
	ret     8
__OVRINIT   ENDP

; Swaps the trap interrupt's vector with TrapVector, then closes the
; executable if it is open, else reopens it by the name found at startup.
SwapTrap    PROC FAR
	push    bp
	mov     bp, sp
	push    ds
	mov     ds, cs:__OVRGROUP
	mov     al, byte ptr [__OVRTRAP__+1]
	mov     ah, 35h                     ; get vector
	int     21h
	push    es
	push    bx
	push    ds
	mov     al, byte ptr [__OVRTRAP__+1]
	mov     dx, word ptr TrapVector
	mov     ds, word ptr [TrapVector+2]
	mov     ah, 25h                     ; set vector
	int     21h
	pop     ds
	pop     word ptr TrapVector
	pop     word ptr [TrapVector+2]
	cmp     word ptr __OvrDosHandle, 0
	je      @@reopen
	mov     bx, __OvrDosHandle
	mov     ah, 3Eh                     ; close
	int     21h
	mov     word ptr __OvrDosHandle, 0
	jmp     short @@done
@@reopen:
	NOSMART
	lea     dx, __OVRFILENAME
	SMART
	mov     ax, 3D00h                   ; the library opens with AL = OpenMode
	int     21h
	mov     __OvrDosHandle, ax
@@done:
	pop     ds
	pop     bp
	ret
SwapTrap    ENDP

; Unhooks the trap interrupt, closes the executable and calls the
; EMS and extended memory exit hooks.
__OVREXIT   PROC FAR
	push    bp
	mov     bp, sp
	push    ds
	mov     ds, cs:__OVRGROUP
	cmp     word ptr __OvrDosHandle, 0
	je      @@closed
	NOSMART
	call    far ptr SwapTrap
	SMART
@@closed:
	push    cs
	call    word ptr __CALLEMSEXIT
	push    cs
	call    word ptr __CALLEXTEXIT
	pop     ds
	pop     bp
	ret
__OVREXIT   ENDP

; Opens fileName as given.
OpenInCurrentDir    PROC NEAR
	push    ds
	NOSMART
	lea     di, __OVRFILENAME
	SMART
	push    ds
	pop     es
	call    AppendAndOpen
	pop     ds
	ret
OpenInCurrentDir    ENDP

; Under DOS 3 and later, looks for the file in the program's own directory,
; taken from the end of the environment.
OpenFromExeDir  PROC NEAR
	mov     ah, 30h                     ; DOS version
	int     21h
	cmp     al, 3
	jb      @@done                      ; the library sets OpenMode to 20h (deny write) after this
	push    ds
	mov     ax, DGROUP
	mov     ds, ax
	ASSUME  DS:DGROUP
	mov     ds, __envseg
	xor     si, si
	cld
@@skipVar:
	lodsb
	or      al, al
	jne     @@skipVar
	lodsb
	or      al, al
	jne     @@skipVar
	lodsw                               ; the string count before the path
	ASSUME  DS:_OVRGROUP_

	; Copy the path, remembering where its last directory ends.
	NOSMART
	lea     di, __OVRFILENAME
	SMART
	mov     ax, _OVRGROUP_
	mov     es, ax
	mov     bx, di
@@copyPath:
	lodsb
	stosb
	or      al, al
	je      @@endPath
	cmp     al, '\'
	jne     @@copyPath
	mov     bx, di
	jmp     @@copyPath
@@endPath:
	mov     di, bx
	call    AppendAndOpen
	pop     ds
@@done:
	ret
OpenFromExeDir  ENDP

; Looks for the file in each directory of PATH.
OpenFromPath    PROC NEAR
	push    ds
	mov     ax, DGROUP
	mov     ds, ax
	ASSUME  DS:DGROUP
	mov     ds, __envseg
	xor     si, si
@@findPath:
	mov     di, offset PathName
	push    cs
	pop     es
	mov     cx, 5
	cld
	repe    cmpsb
	je      @@nextDir
	dec     si
@@skipVar:
	lodsb
	or      al, al
	jne     @@skipVar
	cmp     al, [si]
	jne     @@findPath
@@notFound:
	pop     ds
	stc
	ret
@@nextDir:
	cmp     byte ptr [si], 0
	je      @@notFound
	ASSUME  DS:_OVRGROUP_
	NOSMART
	lea     di, __OVRFILENAME
	SMART
	mov     ax, _OVRGROUP_
	mov     es, ax
	xor     al, al
@@copyDir:
	mov     ah, al
	lodsb
	or      al, al
	je      @@lastDir
	cmp     al, ';'
	je      @@endDir
	stosb
	jmp     @@copyDir
@@lastDir:
	dec     si
@@endDir:
	cmp     ah, ':'
	je      @@open
	cmp     ah, '\'
	je      @@open
	mov     al, '\'
	stosb
@@open:
	push    ds
	push    si
	call    AppendAndOpen
	pop     si
	pop     ds
	jb      @@nextDir
	pop     ds
	ret
OpenFromPath    ENDP

; Appends fileName (12 characters at most) at es:di and opens the result.
AppendAndOpen   PROC NEAR
	lds     si, fileName
	mov     ax, ds
	or      ax, si
	je      @@open
	mov     cx, 12
@@copy:
	lodsb
	stosb
	or      al, al
	je      @@open
	loop    @@copy
	sub     al, al
	stosb
@@open:
	NOSMART
	lea     dx, __OVRFILENAME
	SMART
	mov     ax, _OVRGROUP_
	mov     ds, ax
	mov     ax, 3D00h                   ; the library opens with AL = OpenMode
	int     21h
	ret
AppendAndOpen   ENDP

; Reads CX bytes into __OVRINIT's header buffer; carry set when short.
ReadHeader  PROC NEAR
	push    ds
	lea     dx, header
	push    ss
	pop     ds
	mov     ah, 3Fh                     ; read
	int     21h
	pop     ds
	jb      @@done
	cmp     ax, cx
@@done:
	ret
ReadHeader  ENDP

; Links every overlaid segment's stub into the code list, moves its file
; position past the FBOV base, and keeps the largest code size.
__InitModules   PROC NEAR
	mov     ax, DGROUP
	mov     es, ax
	mov     ax, es:__psp
	add     ax, 16
	mov     cs:LoadSeg, ax
	mov     ax, _STUB_
	mov     es, ax
	xor     bx, bx
	xor     di, di
	NOSMART
	lea     si, __SEGTABLE__
	SMART
@@nextSeg:
	test    [si].seg_flags, 2
	je      @@skip
	cmp     [si].seg_size, 0
	jne     @@overlaid
@@skip:
	jmp     @@advance

	; Link it after the previous stub; a stub flagged 0FFh is left out.
@@overlaid:
	mov     ax, [si].seg_base
	push    es
	mov     es:[stub_next], ax
	mov     es, ax
	cmp     byte ptr es:[stub_flags], 0FFh
	jne     @@link
	pop     es
	mov     word ptr es:[stub_next], 0
	jmp     @@advance
@@link:
	pop     ax
	mov     word ptr es:[stub_reader], offset __ReadOvrDisk
	mov     ax, __OvrFileBase
	mov     dx, [__OvrFileBase+2]
	add     word ptr es:[stub_filepos], ax
	adc     word ptr es:[stub_filepos+2], dx
	call    LoadParas
	cmp     bx, dx
	jae     @@advance
	xchg    bx, dx
@@advance:
	add     si, SIZE SEGENTRY
	cmp     si, offset _OVRGROUP_:__SEGTABEND__
	jae     @@done
	jmp     @@nextSeg
@@done:
	xor     ax, ax
	add     bx, 2
	mov     __OvrMinHeapSize, bx
	ret
__InitModules   ENDP

; Loads, in one read, as many overlays as fit end to end in the heap,
; then fixes up each one and reports it to the hook. Carry on a read error.
LoadAtStart PROC NEAR
	mov     cx, __OvrCodeList
	mov     __OvrLoadList, cx
	mov     bx, cx
	mov     si, __OvrHeapOrg
	mov     di, __OvrHeapEnd
	push    ds

	; Each overlay's place in the heap follows from its file position.
@@place:
	mov     ds, cx
	mov     cx, ds:[stub_next]
	jcxz    @@placed
	mov     es, cx
	mov     ax, word ptr es:[stub_filepos]
	mov     dx, word ptr es:[stub_filepos+2]
	sub     ax, word ptr ds:[stub_filepos]
	sbb     dx, word ptr ds:[stub_filepos+2]
	mov     cx, 16
	div     cx
	add     ax, si
	cmp     ax, di
	ja      @@placed
	mov     ds:[stub_loadseg], si
	mov     si, ax
	mov     cx, es
	mov     ds:[stub_nextload], cx
	mov     bx, ds
	jmp     @@place
@@placed:
	mov     ds, bx
	mov     word ptr ds:[stub_nextload], 0
	pop     ds
	mov     __OvrHeapPtr, si
	mov     ax, __OvrHeapOrg
	sub     si, ax
	jne     @@read
	jmp     @@ok

	; Paragraphs to bytes, in DI:SI.
@@read:
	mov     cl, 4
	rol     si, cl
	mov     di, si
	and     di, 0Fh
	and     si, -10h
	mov     es, __OvrLoadList
	mov     dx, word ptr es:[stub_filepos]
	mov     cx, word ptr es:[stub_filepos+2]
	call    ReadBlock
	jb      @@done
	mov     ax, __OvrLoadList
	push    ds
@@fixup:
	mov     es, ax
	mov     cx, es:[stub_fixups]
	jcxz    @@noFixups
	call    ApplyFixups
@@noFixups:
	cmp     word ptr es:[stub_entries], 0
	je      @@noJumps
	call    WriteJumps
@@noJumps:
	mov     ax, es:[stub_loadseg]
	dec     ax
	mov     ds, ax
	mov     ds:[blk_stub], es
	push    es
	mov     ax, 0FFFFh
	mov     bx, ax
	mov     ds, cs:__OVRGROUP
	call    dword ptr __OVRHOOK__
	pop     es
	mov     ax, es:[stub_nextload]
	or      ax, ax
	jne     @@fixup
	pop     ds
@@ok:
	clc
@@done:
	ret
LoadAtStart ENDP

; Byte count in DX:AX to paragraphs. Nothing calls it.
BytesToParas    PROC NEAR
	mov     cl, 4
	shr     ax, cl
	ror     dx, cl
	and     dx, 0F000h
	or      ax, dx
	ret
BytesToParas    ENDP

; Seeks the executable to CX:DX and reads DI:SI bytes to AX:0, in
; chunks of under 64K. Carry on an error or a short read.
ReadBlock   PROC NEAR
	push    ax
	mov     bx, __OvrDosHandle
	mov     ax, 4200h                   ; seek from the start
	int     21h
	pop     ax
	push    ds
	mov     ds, ax
	jmp     short @@chunk
@@nextChunk:
	mov     ax, ds
	add     ax, 0FFFh
	mov     ds, ax
@@chunk:
	mov     cx, 0FFF0h
	or      di, di
	jne     @@read
	mov     cx, si
@@read:
	xor     dx, dx
	mov     ah, 3Fh                     ; read
	int     21h
	jb      @@done
	cmp     ax, cx
	jb      @@done
	sub     si, ax
	sbb     di, 0
	mov     ax, si
	or      ax, di
	jne     @@nextChunk
@@done:
	pop     ds
	ret
ReadBlock   ENDP

; Applies the CX bytes of relocations that follow a loaded overlay's
; code. Each names a word in the code and a segment table entry whose
; segment goes there.
ApplyFixups PROC NEAR
	push    ds
	push    es
	mov     ax, es:[stub_codesize]
	mov     si, ax
	and     si, 0Fh
	shr     ax, 1
	shr     ax, 1
	shr     ax, 1
	shr     ax, 1
	mov     dx, es:[stub_loadseg]
	add     ax, dx
	mov     ds, ax
	mov     es, dx
	shr     cx, 1
	cld
@@next:
	lodsw
	mov     bx, ax
	mov     di, es:[bx]
	push    ds
	mov     ax, _EXEINFO_
	mov     ds, ax
	mov     ax, di
	and     di, -8
	mov     dx, [di].seg_base
	mov     es:[bx], dx
	test    ax, 1
	je      @@plain
	call    PatchFarPush
@@plain:
	pop     ds
	loop    @@next
	pop     es
	pop     ds
	ret
ApplyFixups ENDP

; A relocation inside "mov r, seg; push r; mov r, ofs; push r" pushes
; the address of an overlaid routine: point the offset at the routine's
; jump table entry in stub DX instead.
PatchFarPush    PROC NEAR
	push    cx
	mov     ds, dx
	mov     ah, es:[bx-1]
	mov     al, ah
	and     ax, 0F807h
	cmp     ah, 0B8h                    ; mov r16, imm
	jne     @@done
	mov     cx, ax
	mov     ah, es:[bx+2]
	mov     al, ah
	and     ax, 0F807h
	cmp     ah, 50h                     ; push r16
	jne     @@done
	cmp     al, cl
	jne     @@done
	mov     dx, ax
	mov     ah, es:[bx+3]
	mov     al, ah
	and     ax, 0F807h
	cmp     cx, ax
	jne     @@done
	mov     ah, es:[bx+6]
	mov     al, ah
	and     ax, 0F807h
	cmp     ax, dx
	jne     @@done
	mov     di, stub_jumps
	mov     cx, ds:[stub_entries]
	mov     ax, es:[bx+4]
@@search:
	cmp     ax, [di].tj_target
	je      @@found
	add     di, SIZE TRAPJUMP
	loop    @@search
	jmp     short @@notFound
@@found:
	mov     es:[bx+4], di
@@done:
	pop     cx
	ret
@@notFound:
	pop     cx
	ret
PatchFarPush    ENDP

; The stub's reader: reads the overlay's code and relocations from
; the executable and applies them. Carry on a read error.
__ReadOvrDisk   PROC NEAR
	mov     si, es:[stub_codesize]
	xor     di, di
	add     si, es:[stub_fixups]
	adc     di, 0
	mov     dx, word ptr es:[stub_filepos]
	mov     cx, word ptr es:[stub_filepos+2]
	mov     ax, es:[stub_loadseg]
	call    ReadBlock
	jb      @@done
	mov     cx, es:[stub_fixups]
	jcxz    @@ok
	call    ApplyFixups
@@ok:
	clc
@@done:
	ret
__ReadOvrDisk   ENDP

; The trap interrupt: a call through an unloaded stub lands here. Loads the
; overlay, patches its jump table and resumes the call.
TrapHandler PROC FAR
	push    bp
	mov     bp, sp
	test    bp, 1
	je      @@evenStack
	jmp     far ptr __OverlayHalt
@@evenStack:
	push    ax
	push    bx
	push    cx
	push    dx
	push    si
	push    di
	push    ds
	push    es
	mov     ax, _OVRGROUP_
	mov     ds, ax
	sti

	; Back up to the int 3Fh so it runs again as a jump. At the stub's
	; start it is a return into an unloaded overlay.
	les     bx, dword ptr [bp].sf_ip
	push    word ptr es:[bx]
	sub     [bp].sf_ip, 2
	jne     @@viaJump
	call    LoadOverlay
	jmp     short @@loaded

	; Swap the saved BP above the flags so BP heads a frame with the
	; caller's return address while the stack is walked.
@@viaJump:
	add     bp, 6
	mov     ax, [bp]
	xchg    [bp-6], ax
	mov     [bp], ax
	call    LoadOverlay
	mov     ax, [bp]
	xchg    [bp-6], ax
	mov     [bp], ax
@@loaded:
	pop     bx
	mov     al, es:[stub_flags]
	and     al, 8
	and     byte ptr es:[stub_flags], 0F7h
	cbw
	mov     ds, cs:__OVRGROUP
	call    dword ptr __OVRHOOK__
	pop     es
	pop     ds
	pop     di
	pop     si
	pop     dx
	pop     cx
	pop     bx
	pop     ax
	pop     bp
	iret
TrapHandler ENDP

; Finds heap space for stub ES at __OvrHeapPtr and returns it in AX. The
; oldest overlays in its way are thrown out, or moved behind the others
; while still in use; at the heap's end the rest move to its top.
MakeRoom    PROC NEAR
	push    es
	inc     word ptr __OvrLoadCount
	call    LoadParas
	jmp     short @@check
@@evict:
	popf
	push    dx
	jae     @@takeOldest
	call    PackToTop
@@takeOldest:
	mov     es, __OvrLoadList
	mov     ax, es:[stub_nextload]
	mov     __OvrLoadList, ax
	cmp     byte ptr es:[stub_usage], 0
	jne     @@keep
	call    Unload
	call    CodeParas
	jmp     short @@next
@@keep:
	dec     byte ptr es:[stub_usage]
	call    MoveOverlay
	call    AppendLoaded
@@next:
	pop     dx
@@check:
	call    FreeParas
	pushf
	cmp     dx, ax
	ja      @@evict
	popf
	pop     es
	mov     ax, __OvrHeapPtr
	mov     es:[stub_loadseg], ax
	ret
MakeRoom    ENDP

; Makes stub ES's overlay resident and turns its jump entries into far
; jumps, then re-arms the traps of unused overlays at the head of the
; load list until they and the free space reach __OvrRetrySize paragraphs.
LoadOverlay PROC NEAR
	inc     word ptr __OvrTrapCount
	cmp     word ptr es:[stub_loadseg], 0
	je      @@notLoaded
	mov     byte ptr es:[stub_usage], 1
	or      byte ptr es:[stub_flags], 4
	jmp     @@loaded
@@notLoaded:
	or      byte ptr es:[stub_flags], 8
	call    MakeRoom
	push    ds
	dec     ax
	mov     ds, ax
	mov     ds:[blk_stub], es
	pop     ds
	call    word ptr es:[stub_reader]
	jb      @@readFailed
	call    AppendLoaded
@@loaded:
	call    EnableJumps
	mov     al, es:[stub_flags]
	and     al, 3
	add     es:[stub_usage], al
	push    es
	call    FreeParas
	mov     es, __OvrLoadList
@@probation:
	mov     cx, es:[stub_nextload]
	jcxz    @@done
	cmp     ax, __OvrRetrySize
	jae     @@done
	push    cx
	push    ax
	cmp     byte ptr es:[stub_usage], 0
	je      @@rearm
	xor     ax, ax
	jmp     short @@counted
@@rearm:
	call    DisableJumps
	call    CodeParas
@@counted:
	pop     cx
	pop     es
	add     ax, cx
	jmp     @@probation
@@done:
	pop     es
	ret
@@readFailed:
	jmp     far ptr __OverlayHalt
LoadOverlay ENDP

; Throws out stub ES's overlay, handing it to the swap hook when it was
; read from disk.
Unload  PROC NEAR
	call    DisableJumps
	cmp     word ptr es:[stub_reader], offset __ReadOvrDisk
	jne     @@forget
	call    word ptr __CALLEXTEMSSWAP
@@forget:
	mov     word ptr es:[stub_loadseg], 0
	ret
Unload  ENDP

; Moves every loaded overlay, in order, to the top of the heap and
; restarts __OvrHeapPtr at its bottom.
PackToTop   PROC NEAR
	mov     ax, __OvrLoadList
	xor     cx, cx
@@stack:
	inc     cx
	push    ax
	mov     es, ax
	mov     ax, es:[stub_nextload]
	or      ax, ax
	jne     @@stack
	mov     __OvrLoadList, ax
	mov     ax, __OvrHeapEnd
	mov     __OvrHeapPtr, ax
@@move:
	pop     es
	push    cx
	mov     ax, __OvrLoadList
	mov     es:[stub_nextload], ax
	mov     __OvrLoadList, es
	call    CodeParas
	sub     __OvrHeapPtr, ax
	call    MoveOverlay
	pop     cx
	loop    @@move
	mov     ax, __OvrHeapOrg
	mov     __OvrHeapPtr, ax
	ret
PackToTop   ENDP

; Turns stub ES's jump entries into far jumps to its loaded code, first
; repointing frames that return into the stub at the code.
; LoadAtStart enters at WriteJumps.
EnableJumps PROC NEAR
	cmp     word ptr es:[stub_entries], 0
	jne     @@hasEntries
	ret
@@hasEntries:
	cmp     byte ptr es:[stub_jumps], OP_JMPFAR
	je      @@done
	mov     cx, es:[stub_retofs]
	jcxz    WriteJumps
	mov     ax, es:[stub_loadseg]
	mov     dx, es
	call    SwapFrameSegs
WriteJumps:
	mov     bx, es:[stub_loadseg]
	mov     cx, es:[stub_entries]
	mov     di, stub_jumps
	cld
@@next:
	mov     dx, es:[di].tj_target
	mov     al, OP_JMPFAR
	stosb
	mov     ax, dx
	stosw
	mov     ax, bx
	stosw
	loop    @@next
@@done:
	ret
EnableJumps ENDP

; Turns stub ES's jump entries back into traps, and points frames that
; return into its code at the stub, so that returning reloads it.
DisableJumps    PROC NEAR
	cmp     byte ptr es:[stub_jumps], OP_INT
	je      @@done
	mov     ax, es
	mov     dx, es:[stub_loadseg]
	xor     cx, cx
	call    SwapFrameSegs
	mov     es:[stub_retofs], cx
	mov     cx, es:[stub_entries]
	mov     di, stub_jumps
	cld
@@next:
	mov     dx, es:[di].fj_offset
	mov     ax, __OVRTRAP__
	stosw
	mov     ax, dx
	stosw
	xor     al, al
	stosb
	loop    @@next
@@done:
	ret
DisableJumps    ENDP

; Moves stub ES's overlay to __OvrHeapPtr, copying in whichever
; direction is safe, and repoints the frames and jumps that use it.
MoveOverlay PROC NEAR
	mov     ax, __OvrHeapPtr
	mov     dx, es:[stub_loadseg]
	mov     es:[stub_loadseg], ax
	mov     cx, es:[stub_codesize]
	inc     cx
	shr     cx, 1
	xor     si, si
	cld
	cmp     ax, dx
	jb      @@copy
	mov     si, cx
	dec     si
	shl     si, 1
	std
@@copy:
	mov     di, si
	push    ds
	push    es
	mov     ds, dx
	mov     es, ax
	rep     movsw
	cld
	dec     ax
	mov     ds, ax
	pop     es
	mov     ds:[blk_stub], es
	inc     ax
	pop     ds
	cmp     byte ptr es:[stub_jumps], OP_INT
	je      @@done
	call    FixFrameSegs
	mov     cx, es:[stub_entries]
	mov     di, stub_jumps + fj_segment
	cld
@@next:
	stosw
	add     di, 3
	loop    @@next
@@done:
	ret
MoveOverlay ENDP

; Adds stub ES's code to __OvrHeapPtr and appends it to the load list.
AppendLoaded    PROC NEAR
	call    CodeParas
	add     __OvrHeapPtr, ax
	push    ds
	mov     ax, _STUB_
@@walk:
	mov     ds, ax
	mov     ax, ds:[stub_nextload]
	or      ax, ax
	jne     @@walk
	mov     ds:[stub_nextload], es
	mov     es:[stub_nextload], ax
	pop     ds
	ret
AppendLoaded    ENDP

; FixFrameSegs, then swaps CX with the return offset of the first
; frame that matched.
SwapFrameSegs   PROC NEAR
	call    FixFrameSegs
	or      bx, bx
	je      @@done
	xchg    ss:[bx].sf_ip, cx
@@done:
	ret
SwapFrameSegs   ENDP

; Walks the BP chain, making frames that return to segment DX return
; to AX instead. BX is the first one changed, or 0.
FixFrameSegs    PROC NEAR
	xor     bx, bx
	push    cx
	push    bp
	jmp     short @@frame
@@next:
	shl     cx, 1
	mov     bp, cx
@@frame:
	mov     cx, [bp].sf_bp
	shr     cx, 1
	je      @@done
	jb      @@next
	cmp     dx, [bp].sf_cs
	jne     @@next
	mov     [bp].sf_cs, ax
	or      bx, bx
	jne     @@next
	mov     bx, bp
	jmp     @@next
@@done:
	pop     bp
	pop     cx
	ret
FixFrameSegs    ENDP

; Paragraphs free from __OvrHeapPtr up to the oldest loaded overlay,
; or, with carry, up to the heap's end.
FreeParas   PROC NEAR
	mov     ax, __OvrLoadList
	or      ax, ax
	je      @@toEnd
	mov     es, ax
	mov     ax, es:[stub_loadseg]
	sub     ax, __OvrHeapPtr
	jae     @@done
@@toEnd:
	mov     ax, __OvrHeapEnd
	sub     ax, __OvrHeapPtr
	stc
@@done:
	ret
FreeParas   ENDP

; Paragraphs stub ES's code takes in the heap, with its block header.
CodeParas   PROC NEAR
	mov     ax, es:[stub_codesize]
	add     ax, 11h
	mov     cl, 4
	shr     ax, cl
	ret
CodeParas   ENDP

; CodeParas in AX, and in DX that plus the relocations' paragraphs.
LoadParas   PROC NEAR
	mov     cl, 4
	mov     ax, es:[stub_codesize]
	add     ax, 11h
	shr     ax, cl
	mov     dx, es:[stub_fixups]
	add     dx, 0Fh
	shr     dx, cl
	add     dx, ax
	ret
LoadParas   ENDP

; Returns the stub of the overlay loaded at segment AX, or 0 with
; carry when AX is not a loaded overlay.
__ISOVERLAYMOD  PROC FAR
	push    ds
	push    es
	mov     ds, cs:__OVRGROUP
	cmp     ax, __OvrHeapOrg
	jb      @@no
	cmp     ax, __OvrHeapEnd
	jae     @@no
	dec     ax
	mov     es, ax
	mov     es, es:[blk_stub]
	inc     ax
	mov     bx, __OVRTRAP__
	cmp     bx, es:[stub_trap]
	jne     @@no
	cmp     ax, es:[stub_loadseg]
	mov     ax, es
	je      @@done
@@no:
	xor     ax, ax
	stc
@@done:
	pop     es
	pop     ds
	ret
__ISOVERLAYMOD  ENDP

; Walks the stack frames below CX. A frame returning to a stub's start
; clears that stub's parked offset; if any did, frames returning elsewhere
; into a stub with none park their offset there and return to its start.
__CHECKSTACK    PROC FAR
	LOCAL   found:BYTE = checkFrame
	push    bp
	mov     bp, sp
	sub     sp, checkFrame
	mov     found, 0
	push    ds
	mov     ds, cs:__OVRGROUP
	push    si
	push    di
	mov     bx, bp
	mov     ax, __OVRTRAP__
	mov     dx, __OvrHeapOrg
	jmp     short @@frame
@@next:
	shl     si, 1
	mov     bx, si
@@frame:
	cmp     bx, cx
	jae     @@walked
	mov     si, ss:[bx].sf_bp
	shr     si, 1
	je      @@walked
	jb      @@next
	mov     di, ss:[bx].sf_cs
	cmp     di, dx
	jae     @@next
	mov     es, di
	mov     di, ss:[bx].sf_ip
	or      di, di
	jne     @@next
	cmp     es:[di].stub_trap, ax
	jne     @@next
	mov     found, 1
	mov     es:[di].stub_retofs, di
	jmp     @@next
@@walked:
	cmp     found, 1
	je      @@park
	jmp     @@done
@@parkNext:
	shl     si, 1
	mov     bx, si
@@park:
	mov     si, ss:[bx].sf_bp
	shr     si, 1
	je      @@done
	jb      @@parkNext
	mov     di, ss:[bx].sf_cs
	cmp     di, dx
	jae     @@parkNext
	mov     es, di
	mov     di, ss:[bx].sf_ip
	or      di, di
	je      @@parkNext
	cmp     es:[stub_trap], ax
	jne     @@parkNext
	cmp     word ptr es:[stub_retofs], 0
	jne     @@parkNext
	xchg    es:[stub_retofs], di
	mov     ss:[bx].sf_ip, di
	jmp     @@parkNext
@@done:
	pop     di
	pop     si
	pop     ds
	mov     sp, bp
	pop     bp
	ret
__CHECKSTACK    ENDP

; Makes sure the overlay behind the far pointer target is loaded, and
; returns its loaded segment in target's segment half.
__CHECKOVERLAY  PROC FAR
	ARG     target:DWORD
	push    bp
	mov     bp, sp
	push    ds
	mov     ds, cs:__OVRGROUP
	mov     es, word ptr target+2
	mov     cx, __OVRTRAP__
	cmp     cx, es:[stub_trap]
	jne     @@done
	cmp     word ptr es:[stub_loadseg], 0
	je      @@load
	cmp     byte ptr es:[stub_jumps], OP_INT
	jne     @@loaded
@@load:
	mov     word ptr target+2, 0
	push    si
	push    di
	call    LoadOverlay
	pop     di
	pop     si
	mov     bx, word ptr target
	push    es
	mov     al, es:[stub_flags]
	and     al, 8
	and     byte ptr es:[stub_flags], 0F7h
	cbw
	mov     ds, cs:__OVRGROUP
	call    dword ptr __OVRHOOK__
	pop     es
@@loaded:
	mov     ax, es:[stub_loadseg]
	mov     word ptr target+2, ax
@@done:
	pop     ds
	pop     bp
	ret
__CHECKOVERLAY  ENDP
_OVRTEXT_   ENDS

	END
