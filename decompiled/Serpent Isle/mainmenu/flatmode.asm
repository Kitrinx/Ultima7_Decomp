; Serpent Isle MAINMENU.EXE, one module of resident segment 0 (file offsets 0x002a40 to 0x003531, 2801 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.

; Flat real mode for the 386: a brief trip into protected mode loads 4 GB
; limits into ES, FS and GS, which real mode then keeps. The protected-mode
; side has its own GDT, a TSS, and an IDT whose 256 gates all lead to one
; handler that hands the interrupt back to the real-mode vector.

	.386P
	LOCALS
	NOSMART

SEL_TSS         EQU 08h                 ; this task
SEL_TSS2        EQU 10h                 ; spare task
SEL_FLAT        EQU 18h                 ; data, base 0, 4 GB
SEL_LOW64K      EQU 20h                 ; data, base 0, 64 KB
SEL_CODE        EQU 28h                 ; this module's code
SEL_DGROUP      EQU 30h                 ; DGROUP
SEL_CODE64K     EQU 38h                 ; _TEXT, 64 KB

ACC_TSS         EQU 89h                 ; present 386 TSS
ACC_DATA        EQU 92h                 ; present writable data
ACC_CODE        EQU 9Ah                 ; present readable code
GRAN_4G         EQU 0CFh                ; 4 KB pages, 32-bit, limit bits 16-19 set
GATE_INT386     EQU 0EE00h              ; present 386 interrupt gate, callable from ring 3

CR0_PE          EQU 1                   ; protection enable
EFL_IF_TF       EQU 0300h               ; interrupt and trap flags
EFL_VM          EQU 20000h              ; virtual 8086 mode
EFL_IOPL3       EQU 3000h
REFLECTOR_SIZE  EQU 7                   ; push bp / mov bp,N / jmp
INT_RETURN      EQU 63h                 ; raised when a reflected handler has returned

; A segment descriptor: limit, base bits 0-15, access byte, granularity.
DESCRIPTOR MACRO limit, base, access, gran
	dw      limit, base
	db      0, access, gran, 0
ENDM

_TEXT   SEGMENT BYTE PUBLIC USE16 'CODE'
_TEXT   ENDS
_DATA   SEGMENT WORD PUBLIC USE16 'DATA'
_DATA   ENDS
DGROUP  GROUP   _DATA

	PUBLIC  InitFlatMode, SetFlatMode, ClearFlatMode

_DATA   SEGMENT
GdtPtr      dw  GdtEnd - Gdt, Gdt, 0    ; bases are offsets until RelocateBases runs
IdtPtr      dw  IdtEnd - Idt, Idt, 0

Gdt         dq  0
	DESCRIPTOR  <TssEnd - Tss>, Tss, ACC_TSS, 0
	DESCRIPTOR  <Tss2End - Tss2>, Tss2, ACC_TSS, 0
	DESCRIPTOR  0FFFFh, 0, ACC_DATA, GRAN_4G
	DESCRIPTOR  0FFFFh, 0, ACC_DATA, 0
	DESCRIPTOR  CodeEnd, 0, ACC_CODE, 0
	DESCRIPTOR  DataEnd, 0, ACC_DATA, 0
	DESCRIPTOR  0FFFFh, 0, ACC_CODE, 0
GdtEnd      LABEL   BYTE

Idt         dq  256 dup (?)             ; built by BuildIdt
IdtEnd      LABEL   BYTE

Tss         dd  0                       ; back link
TssEsp0     dw  StackTop0, 0
	dd      SEL_DGROUP                  ; ss0
	dd      5 dup (0)                   ; esp1, ss1, esp2, ss2, cr3
	dw      FlatSwitched, 0             ; eip
	dd      EFL_IOPL3                   ; eflags
	dd      4 dup (0)                   ; eax, ecx, edx, ebx
	dw      StackTop, 0                 ; esp
	dd      4 dup (0)                   ; ebp, esi, edi, es
	dd      SEL_CODE                    ; cs
	dd      SEL_DGROUP                  ; ss
	dd      4 dup (0)                   ; ds, fs, gs, ldt
	dw      0                           ; trap
	dw      IoMap - Tss
IoMap       db  8192 dup (0)            ; every port allowed
	db      0FFh
TssEnd      LABEL   BYTE

Tss2        db  66h dup (0)
	dw      RelocTable                  ; past the limit, so no I/O map
	db      0FFh
Tss2End     LABEL   BYTE

; Words that hold a linear base: the segment named beside each is added.
RelocCount  dw  7
RelocTable  dw  GdtPtr+2, DGROUP
	dw      IdtPtr+2, DGROUP
	dw      Gdt+SEL_TSS+2, DGROUP
	dw      Gdt+SEL_TSS2+2, DGROUP
	dw      Gdt+SEL_CODE+2, _TEXT
	dw      Gdt+SEL_DGROUP+2, DGROUP
	dw      Gdt+SEL_CODE64K+2, _TEXT

SavedIF     db  ?                       ; interrupts were enabled
SavedIdt    df  3FFh                    ; the real-mode IDT, 1 KB at 0 until saved
	dw      ?
StackTop0   dw  ?                       ; the TSS's initial stack pointers
StackTop    dw  ?
ReturnIp    dw  ?                       ; iret frame the real-mode handler returns through
ReturnCs    dw  ?
ReturnFlags dw  ?
	dw      47 dup (?)              ; stack while switching modes
SavedEsp    dd  ?                       ; caller's stack
SavedSs     dw  ?
DataEnd     LABEL   BYTE
_DATA   ENDS

_TEXT   SEGMENT
	ASSUME  CS:_TEXT, DS:DGROUP

; Enter flat real mode for the first time: fix up the tables and build the IDT.
; Returns 0 in EAX, or 1 without a 386 or when already in protected mode.
InitFlatMode    PROC FAR
	push    bp
	mov     bp, sp
	push    ebx
	push    ecx
	push    edx
	push    edi
	push    esi
	push    ebp
	push    ds
	push    es
	push    fs
	push    gs
	pushf
	cli
	mov     ax, DGROUP
	mov     ds, ax
	mov     SavedSs, ss
	mov     SavedEsp, esp
	mov     ss, ax
	mov     sp, offset SavedEsp
	call    Is386
	cmp     ax, 0
	jne     short @@is386
	jmp     @@fail
@@is386:
	smsw    ax
	test    ax, CR0_PE
	je      short @@real
@@fail:
	mov     ax, DGROUP
	mov     ds, ax
	mov     ss, SavedSs
	mov     esp, SavedEsp
	popf
	pop     gs
	pop     fs
	pop     es
	pop     ds
	pop     ebp
	pop     esi
	pop     edi
	pop     edx
	pop     ecx
	pop     ebx
	pop     bp
	mov     eax, 1
	ret
@@real:
	sidt    ss:SavedIdt
	call    RelocateBases
	call    BuildIdt
	lidt    fword ptr ss:IdtPtr
	lgdt    fword ptr ss:GdtPtr
	mov     eax, CR0_PE
	mov     cr0, eax
	jmp     short $+2
FlatSwitched:
	mov     ax, SEL_FLAT
	mov     es, ax
	mov     fs, ax
	mov     gs, ax
	mov     ds, ax
	mov     ax, SEL_LOW64K
	mov     ss, ax
	jmp     dword ptr cs:@@protected
@@protected     dw  @@prot, SEL_CODE64K
@@prot:
	cli
	xor     eax, eax
	mov     cr0, eax
	jmp     far ptr @@back
@@back:
	mov     ax, DGROUP
	mov     ds, ax
	mov     ss, ax
	mov     sp, offset SavedEsp
	lidt    SavedIdt
	sti
	xor     eax, eax
	mov     bx, DGROUP
	mov     ds, bx
	mov     ss, SavedSs
	mov     esp, SavedEsp
	popf
	pop     gs
	pop     fs
	pop     es
	pop     ds
	pop     ebp
	pop     esi
	pop     edi
	pop     edx
	pop     ecx
	pop     ebx
	pop     bp
	ret
InitFlatMode    ENDP

; Load the 4 GB limits again, once InitFlatMode has run. Returns as InitFlatMode.
SetFlatMode     PROC FAR
	push    bp
	mov     bp, sp
	push    ebx
	push    ecx
	push    edx
	push    edi
	push    esi
	push    ebp
	push    ds
	push    es
	push    fs
	push    gs
	pushf
	cli
	mov     ax, DGROUP
	mov     ds, ax
	mov     SavedSs, ss
	mov     SavedEsp, esp
	mov     ss, ax
	mov     sp, offset SavedEsp
	call    Is386
	cmp     ax, 0
	jne     short @@is386
	jmp     @@fail
@@is386:
	smsw    ax
	test    ax, CR0_PE
	je      short @@real
@@fail:
	mov     ax, DGROUP
	mov     ds, ax
	mov     ss, SavedSs
	mov     esp, SavedEsp
	popf
	pop     gs
	pop     fs
	pop     es
	pop     ds
	pop     ebp
	pop     esi
	pop     edi
	pop     edx
	pop     ecx
	pop     ebx
	pop     bp
	mov     eax, 1
	ret
@@real:
	sidt    ss:SavedIdt
	lidt    fword ptr ss:IdtPtr
	lgdt    fword ptr ss:GdtPtr
	cli
	mov     eax, CR0_PE
	mov     cr0, eax
	jmp     short $+2
	mov     ax, SEL_FLAT
	mov     es, ax
	mov     fs, ax
	mov     gs, ax
	mov     ds, ax
	mov     ax, SEL_LOW64K
	mov     ss, ax
	jmp     dword ptr cs:@@protected
@@protected     dw  @@prot, SEL_CODE64K
@@prot:
	cli
	xor     eax, eax
	mov     cr0, eax
	jmp     far ptr @@back
@@back:
	mov     ax, DGROUP
	mov     ds, ax
	mov     ss, ax
	mov     sp, offset SavedEsp
	lidt    SavedIdt
	xor     eax, eax
	mov     bx, DGROUP
	mov     ds, bx
	mov     ss, SavedSs
	mov     esp, SavedEsp
	popf
	pop     gs
	pop     fs
	pop     es
	pop     ds
	pop     ebp
	pop     esi
	pop     edi
	pop     edx
	pop     ecx
	pop     ebx
	pop     bp
	ret
SetFlatMode     ENDP

; Put back the 64 KB limits. Returns as InitFlatMode.
ClearFlatMode   PROC FAR
	push    bp
	mov     bp, sp
	push    ebx
	push    ecx
	push    edx
	push    edi
	push    esi
	push    ebp
	push    ds
	push    es
	push    fs
	push    gs
	pushf
	cli
	mov     ax, DGROUP
	mov     ds, ax
	mov     SavedSs, ss
	mov     SavedEsp, esp
	mov     ss, ax
	mov     sp, offset SavedEsp
	call    Is386
	cmp     ax, 0
	jne     short @@is386
	jmp     @@fail
@@is386:
	smsw    ax
	test    ax, CR0_PE
	je      short @@real
@@fail:
	mov     eax, 1
	jmp     @@exit
@@real:
	sidt    SavedIdt
	lidt    fword ptr IdtPtr
	lgdt    fword ptr GdtPtr
	cli
	mov     eax, CR0_PE
	mov     cr0, eax
	jmp     short $+2
	mov     ax, SEL_LOW64K
	mov     es, ax
	mov     fs, ax
	mov     gs, ax
	mov     ds, ax
	mov     ss, ax
	jmp     dword ptr cs:@@protected
@@protected     dw  @@prot, SEL_CODE64K
@@prot:
	cli
	xor     eax, eax
	mov     cr0, eax
	jmp     far ptr @@back
@@back:
	mov     ax, DGROUP
	mov     ds, ax
	mov     ss, ax
	mov     sp, offset SavedEsp
	lidt    SavedIdt
	xor     eax, eax
@@exit:
	mov     bx, DGROUP
	mov     ds, bx
	mov     ss, SavedSs
	mov     esp, SavedEsp
	popf
	pop     gs
	pop     fs
	pop     es
	pop     ds
	pop     ebp
	pop     esi
	pop     edi
	pop     edx
	pop     ecx
	pop     ebx
	pop     bp
	ret
ClearFlatMode   ENDP

; One entry per interrupt: BP carries the number to Reflect.
Reflectors      LABEL   NEAR
INT_NUMBER = 0
	REPT    256
	push    bp
	mov     bp, INT_NUMBER
	IF INT_NUMBER EQ INT_RETURN
	jmp     ReflectReturn
	ELSE
	jmp     Reflect
	ENDIF
INT_NUMBER = INT_NUMBER + 1
	ENDM

; Hand an interrupt to its real-mode vector. From virtual 8086 mode the
; iret frame is pushed on the caller's stack; from protected mode the
; handler runs in virtual 8086 mode and comes back through ReturnIp.
Reflect         PROC NEAR
	push    eax
	mov     ax, bp
	mov     bp, sp
	add     bp, 2
	push    ebx
	test    word ptr [bp+0Eh], EFL_VM SHR 16
	je      @@fromProtected
	push    ax
	mov     bx, SEL_DGROUP
	mov     ds, bx
	mov     ax, SEL_FLAT
	mov     fs, ax
	movzx   eax, word ptr [bp+10h]      ; caller's sp
	sub     ax, 6
	mov     [bp+10h], ax
	movzx   ebx, word ptr [bp+14h]      ; caller's ss
	shl     ebx, 4
	add     ebx, eax
	mov     ax, [bp+4]
	mov     fs:[ebx], ax
	mov     ax, [bp+8]
	mov     fs:[ebx+2], ax
	mov     ax, [bp+0Ch]
	mov     fs:[ebx+4], ax
	pop     bx
	shl     bx, 2
	mov     ax, fs:[bx]                 ; real-mode vector
	mov     [bp+4], ax
	mov     ax, fs:[bx+2]
	mov     [bp+8], ax
	and     word ptr [bp+0Ch], NOT EFL_IF_TF
	pop     ebx
	pop     eax
	pop     bp
	iretd
@@fromProtected:
	push    ds
	push    es
	push    fs
	push    gs
	movzx   ebx, ax
	mov     ax, SEL_DGROUP
	mov     ds, ax
	mov     ax, SEL_FLAT
	mov     fs, ax
	push    word ptr TssEsp0
	mov     word ptr TssEsp0, sp
	xor     eax, eax
	mov     ax, DGROUP
	push    eax                         ; gs
	push    eax                         ; fs
	push    eax                         ; ds
	push    eax                         ; es
	push    eax                         ; ss
	mov     ReturnIp, offset IntReturn
	mov     ReturnCs, seg IntReturn
	push    0
	mov     ax, offset ReturnIp
	push    ax                          ; esp
	mov     eax, [bp+0Ch]
	test    ax, 200h
	setne   SavedIF
	and     ax, NOT EFL_IF_TF
	mov     ReturnFlags, ax
	or      eax, EFL_VM
	push    eax                         ; eflags
	mov     eax, fs:[ebx*4]
	sub     esp, 8
	movzx   ebx, ax
	mov     [esp], ebx                  ; eip
	shr     eax, 16
	movzx   ebx, ax
	mov     [esp+4], ebx                ; cs
	mov     ebx, [bp-6]
	mov     eax, [bp-2]
	iretd
Reflect         ENDP

; INT_RETURN brings the real-mode handler back here, to resume the
; protected-mode code it interrupted.
ReflectReturn   PROC NEAR
	push    SEL_DGROUP
	pop     ds
	mov     sp, word ptr TssEsp0
	pop     word ptr TssEsp0
	pushf
	pop     bp
	test    SavedIF, 0                  ; always clear, so IF comes from the handler
	je      short @@flags
	or      bp, 200h
@@flags:
	pop     gs
	pop     fs
	pop     es
	pop     ds
	add     sp, 8
	mov     [esp+0Ah], bp
	pop     bp
	iretd
IntReturn:
	int     INT_RETURN
ReflectReturn   ENDP

; Add each segment's linear address to the base words listed in RelocTable.
RelocateBases   PROC NEAR
	mov     si, offset RelocTable
	mov     cx, RelocCount
	cld
@@next:
	lodsw
	mov     bx, ax
	lodsw
	movzx   edx, ax
	shl     edx, 4
	shld    eax, edx, 16
	add     [bx], dx
	adc     [bx+2], al
	loop    @@next
	ret
RelocateBases   ENDP

; Point every IDT gate at its reflector entry.
BuildIdt        PROC NEAR
	mov     ax, DGROUP
	mov     es, ax
	cld
	mov     dx, offset Reflectors
	mov     di, offset Idt
	mov     cx, 256
@@next:
	mov     ax, dx
	add     dx, REFLECTOR_SIZE
	stosw
	mov     ax, SEL_CODE
	stosw
	mov     ax, GATE_INT386
	stosw
	xor     ax, ax
	stosw
	loop    @@next
	ret
BuildIdt        ENDP

; AX = 1 on a 386 or later: an 8086 pushes SP after decrementing it, and a
; 286 in real mode keeps flag bits 12-14 clear.
Is386           PROC NEAR
	xor     ax, ax
	push    sp
	pop     bx
	cmp     bx, sp
	jne     short @@done
	mov     bx, 7000h
	push    bx
	popf
	pushf
	pop     bx
	and     bx, 7000h
	je      short @@done
	inc     ax
@@done:
	ret
Is386           ENDP
CodeEnd         LABEL   BYTE
_TEXT   ENDS

	END
