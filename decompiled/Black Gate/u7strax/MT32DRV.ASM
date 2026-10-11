; U7STRAX.DRV record 0 (MT32DRV): the Roland MT-32 music driver (MPU-401 in UART mode), 1207
; bytes, loaded at offset 0 of its own segment. Turbo Assembler 2.51 and TLINK /t rebuild it byte
; for byte. The game calls the entries in the table at the start; the far ones take Pascal
; arguments.

	LOCALS

; TLINK /t wants an entry point at 100h; the record itself starts the file.
start	segment para public 'START'
	org	100h
ComEntry:
start	ends

code	segment para public 'CODE'
	assume	cs:code, ds:code

	dw	0
	dw	offset Describe, offset Init, offset Shutdown, offset Timer, offset Note
	dw	offset Control, offset Pitch, offset Program, offset SetPort, offset Sysex

; What Describe returns; it takes the place of the two entries this driver lacks.
DriverInfo	db	0
	db	0                               ; keep the timbre bank
OldTimer	dd	0
SpareHook	dd	0
	dd	0                               ; hook an interrupt
	dd	0                               ; delay
FirstFree	dw	0                      ; first timbre slot after the bank
MpuPort	dw	330h
	db	0
Banner	db	'Origin Sound System!'
DriverTitle	db	'<<<  MQ Driver  >>> '
BankPatch	db	2, 0, 24, 50, 24, 0, 1, 0
CallerDS	dw	0                       ; the game's DS, saved by Init
	db	8 dup (0)

; WaitTicks(high, low): waits that many ticks of the 1.19 MHz timer, as a 32-bit count.
WaitTicks	proc	near
	push	bp
	mov	bp, sp
	push	si
	push	di
	push	bp
	mov	dx, [bp+6]
	mov	bp, [bp+4]
	xor	si, si
	xor	di, di
	mov	al, 6
	out	43h, al
	mov	al, 36h
	in	al, 40h
	mov	bl, al
	in	al, 40h
	mov	bh, al
@@85:
	mov	cx, bx
	mov	al, 6
	out	43h, al
	mov	al, 36h
	in	al, 40h
	mov	bl, al
	in	al, 40h
	mov	bh, al
	sub	cx, bx
	jnc	@@9d
	add	cx, 4DAEh
@@9d:
	add	si, cx
	adc	di, 0
	cmp	di, dx
	jc	@@85
	ja	@@ac
	cmp	si, bp
	jc	@@85
@@ac:
	pop	bp
	pop	di
	pop	si
	pop	bp
	ret	4
WaitTicks	endp

; Left in the driver information as a spare hook; does nothing.
FarReturn	proc	far
	retf
FarReturn	endp

; SendSysex(data, length, address): Roland data set messages of up to 256 bytes each,
; the address bytes holding 7 bits each; waits 80 ms after each and 160 ms after the
; last.
SendSysex	proc	near
	push	bp
	mov	bp, sp
	sub	sp, 4
	push	si
	push	di
	push	ds
@@bd:
	cmp	word ptr [bp+8], 0
	jnz	@@c6
	jmp	@@19a
@@c6:
	cmp	word ptr [bp+8], 100h
	ja	@@d2
	mov	ax, [bp+8]
	jmp	short @@d5
@@d2:
	mov	ax, 100h
@@d5:
	mov	[bp-4], ax
	sub	[bp+8], ax
	mov	word ptr [bp-2], 0
	mov	ax, 0F0h
	push	ax
	call	MpuWrite
	mov	ax, 41h
	push	ax
	call	MpuWrite
	mov	ax, 10h
	push	ax
	call	MpuWrite
	mov	ax, 16h
	push	ax
	call	MpuWrite
	mov	ax, 12h
	push	ax
	call	MpuWrite
	mov	al, [bp+0Ch]
	and	ax, 7Fh
	push	ax
	add	[bp-2], ax
	call	MpuWrite
	mov	al, [bp+0Bh]
	and	ax, 7Fh
	push	ax
	add	[bp-2], ax
	call	MpuWrite
	mov	al, [bp+0Ah]
	and	ax, 7Fh
	push	ax
	add	[bp-2], ax
	call	MpuWrite
	mov	di, [bp-4]
	les	si, dword ptr [bp+4]
@@130:
	mov	al, es:[si]
	inc	si
	push	ax
	add	[bp-2], ax
	call	MpuWrite
	dec	di
	jnz	@@130
	mov	ax, [bp-2]
	neg	ax
	and	ax, 7Fh
	push	ax
	call	MpuWrite
	mov	ax, 0F7h
	push	ax
	call	MpuWrite
	push	ds
	mov	ax, 1
	push	ax
	mov	ax, 7318h
	push	ax
	mov	ds, word ptr cs:CallerDS
	call	WaitTicks
	pop	ds
	mov	ax, [bp+0Ah]
	mov	dx, [bp+0Ch]
	and	ax, 7F7Fh
	and	dx, 7Fh
	shl	al, 1
	shr	ax, 1
	ror	dx, 1
	ror	dx, 1
	add	ah, dh
	xor	dh, dh
	add	ax, [bp-4]
	adc	dx, 0
	mov	dh, ah
	and	dh, 0C0h
	and	ah, 3Fh
	rol	dx, 1
	rol	dx, 1
	shl	ax, 1
	shr	al, 1
	mov	[bp+0Ah], ax
	mov	[bp+0Ch], dx
	jmp	@@bd
@@19a:
	mov	ax, 2
	push	ax
	mov	ax, 0E630h
	push	ax
	mov	ds, word ptr cs:CallerDS
	call	WaitTicks
	pop	ds
	pop	di
	pop	si
	mov	sp, bp
	pop	bp
	ret	0Ah
SendSysex	endp

; Sysex(address, length, data)
Sysex	proc	far
	push	bp
	mov	bp, sp
	push	si
	push	di
	push	ds
	mov	ax, cs
	mov	ds, ax
	push	word ptr [bp+0Eh]
	push	word ptr [bp+0Ch]
	push	word ptr [bp+0Ah]
	push	word ptr [bp+8]
	push	word ptr [bp+6]
	call	SendSysex
	pop	ds
	pop	di
	pop	si
	pop	bp
	retf	0Ah
Sysex	endp

; LoadBankTimbre(timbre, slot, program): the timbre into memory slot (from 1), then a
; patch pointing program (from 1) at it.
LoadBankTimbre	proc	near
	push	bp
	mov	bp, sp
	push	si
	push	di
	push	ds
	dec	word ptr [bp+8]
	and	word ptr [bp+8], 3Fh
	dec	word ptr [bp+0Ah]
	and	word ptr [bp+0Ah], 7Fh
	mov	ax, [bp+8]
	mov	BankPatch+1, al
	mov	ax, 8
	push	ax
	mov	ah, [bp+8]
	shl	ah, 1
	xor	al, al
	push	ax
	mov	ax, 0F6h
	push	ax
	push	word ptr [bp+6]
	push	word ptr [bp+4]
	call	SendSysex
	mov	ax, 5
	push	ax
	mov	ax, [bp+0Ah]
	shl	ax, 1
	shl	ax, 1
	shl	ax, 1
	shl	ax, 1
	shr	al, 1
	push	ax
	mov	ax, 8
	push	ax
	push	cs
	mov	ax, offset BankPatch
	push	ax
	call	SendSysex
	pop	ds
	pop	di
	pop	si
	pop	bp
	ret	8
LoadBankTimbre	endp

; Returns the driver information in DX:AX.
Describe	proc	far
	mov	ax, offset DriverInfo
	mov	dx, cs
	retf
Describe	endp

; Init(timbres): finds the MPU-401 at 330h, else 300h, and puts it in UART mode. Resets
; the synth, then loads the bank: a count, then per timbre its 246 bytes and its program.
; Returns 1, or 0 with no MPU-401.
Init	proc	far
	push	bp
	mov	bp, sp
	sub	sp, 4
	push	si
	push	di
	push	ds
	mov	word ptr cs:CallerDS, ds
	mov	ax, cs
	mov	ds, ax
	mov	ax, offset FarReturn
	mov	word ptr SpareHook, ax
	mov	word ptr SpareHook+2, cs
	mov	cx, 0FFFFh
	call	ResetMpu
	cmp	ax, 0
	jz	@@26e
        mov     word ptr MpuPort, 300h  ; try 300h
	call	ResetMpu
	cmp	ax, 0
	jz	@@26e
	xor	ax, ax
	jmp	@@2e7
@@26e:
	mov	ax, 3Fh
	push	ax
	call	MpuCommand
	mov	ax, 7Fh
	push	ax
	push	ax
        mov     ax, 1                   ; byte 1 of this record, 0
	push	ax
	push	cs
	push	ax
	call	SendSysex
	mov	ax, 20h
	push	ax
	xor	ax, ax
	push	ax
	mov	ax, 14h
	push	ax
	push	cs
	mov	ax, offset Banner
	push	ax
	call	SendSysex
	cmp	word ptr [bp+8], 0
	jz	@@2d1
	les	si, dword ptr [bp+6]
	mov	al, es:[si]
	xor	ah, ah
	inc	si
	mov	[bp-4], ax
	mov	word ptr [bp-2], 1
@@2ad:
	push	es
	mov	al, es:[si+0F6h]
	xor	ah, ah
	push	ax
	push	word ptr [bp-2]
	inc	word ptr [bp-2]
	push	es
	push	si
	call	LoadBankTimbre
	pop	es
	add	si, 0F7h
	dec	word ptr [bp-4]
	jnz	@@2ad
	mov	ax, [bp-2]
	mov	FirstFree, ax
@@2d1:
	mov	ax, 20h
	push	ax
	xor	ax, ax
	push	ax
	mov	ax, 14h
	push	ax
	push	cs
	mov	ax, offset DriverTitle
	push	ax
	call	SendSysex
	mov	ax, 1
@@2e7:
	pop	ds
	add	sp, 0Ch
	pop	di
	pop	si
	mov	sp, bp
	pop	bp
	retf	4
Init	endp

; SetPort(port): the MPU-401 is found by Init instead.
SetPort	proc	far
	push	bp
	mov	bp, sp
	pop	bp
	retf	2
SetPort	endp

; Note(channel, note, velocity): channel from 1; velocity 0 stops the note.
Note	proc	far
	push	bp
	mov	bp, sp
	push	si
	push	di
	push	ds
	mov	ax, cs
	mov	ds, ax
	mov	ax, [bp+0Ah]
	dec	ax
	and	ax, 0Fh
	add	ax, 90h
	push	ax
	call	MpuWrite
	mov	ax, [bp+8]
	and	ax, 7Fh
	push	ax
	call	MpuWrite
	mov	ax, [bp+6]
	and	ax, 7Fh
	push	ax
	call	MpuWrite
	pop	ds
	pop	di
	pop	si
	pop	bp
	retf	6
Note	endp

; Control(channel, control, value)
Control	proc	far
	push	bp
	mov	bp, sp
	push	si
	push	di
	push	ds
	mov	ax, cs
	mov	ds, ax
	mov	ax, [bp+0Ah]
	dec	ax
	and	ax, 0Fh
	add	ax, 0B0h
	push	ax
	call	MpuWrite
	mov	ax, [bp+8]
	and	ax, 7Fh
	push	ax
	call	MpuWrite
	mov	ax, [bp+6]
	and	ax, 7Fh
	push	ax
	call	MpuWrite
	pop	ds
	pop	di
	pop	si
	pop	bp
	retf	6
Control	endp

; Shutdown: all notes off on channels 2-10, then the MPU-401 reset.
Shutdown	proc	far
	push	bp
	mov	bp, sp
	sub	sp, 2
	push	si
	push	di
	push	ds
	mov	ax, cs
	mov	ds, ax
	mov	word ptr [bp-2], 2
@@372:
	push	word ptr [bp-2]
	mov	ax, 7Bh
	push	ax
	xor	ax, ax
	push	ax
	push	cs
	call	near ptr Control
	inc	word ptr [bp-2]
	cmp	word ptr [bp-2], 0Ah
	jna	@@372
	call	ResetMpu
	pop	ds
	pop	di
	pop	si
	mov	sp, bp
	pop	bp
	retf
Shutdown	endp

; Pitch(channel, low, high)
Pitch	proc	far
	push	bp
	mov	bp, sp
	push	si
	push	di
	push	ds
	mov	ax, cs
	mov	ds, ax
	mov	ax, [bp+0Ah]
	dec	ax
	and	ax, 0Fh
	add	ax, 0E0h
	push	ax
	call	MpuWrite
	mov	ax, [bp+8]
	and	ax, 7Fh
	push	ax
	call	MpuWrite
	mov	ax, [bp+6]
	and	ax, 7Fh
	push	ax
	call	MpuWrite
	pop	ds
	pop	di
	pop	si
	pop	bp
	retf	6
Pitch	endp

; Program(channel, program)
Program	proc	far
	push	bp
	mov	bp, sp
	push	si
	push	di
	push	ds
	mov	ax, cs
	mov	ds, ax
	mov	ax, [bp+8]
	dec	ax
	and	ax, 0Fh
	add	ax, 0C0h
	push	ax
	call	MpuWrite
	mov	ax, [bp+6]
	and	ax, 7Fh
	push	ax
	call	MpuWrite
	pop	ds
	pop	di
	pop	si
	pop	bp
	retf	4
Program	endp

UartMode	db	0

; ResetMpu: AX = 0 if the MPU-401 acknowledged a reset, else FFFFh.
ResetMpu	proc	near
	pushf
	cli
	mov	dx, MpuPort
	inc	dx
	mov	al, 0FFh
	out	dx, al
	call	MpuRead
	cmp	ax, 0FEh
	jz	@@417
	mov	dx, MpuPort
	inc	dx
	mov	al, 0FFh
	out	dx, al
	call	MpuRead
	cmp	ax, 0FEh
	jz	@@417
	mov	ax, 0FFFFh
	popf
	ret
@@417:
	mov	ax, 0
	mov	byte ptr UartMode, 0
	popf
	ret
ResetMpu	endp

; MpuCommand(command): AX = FFFFh if the MPU-401 never became ready.
MpuCommand	proc	near
	push	bp
	mov	bp, sp
	push	cx
	pushf
	cli
	mov	cx, 0FFFFh
@@42a:
	mov	dx, MpuPort
	inc	dx
	in	al, dx
	test	al, 40h
	jz	@@43f
	loop	@@42a
	mov	ax, 0FFFFh
	popf
	pop	cx
	pop	bp
	ret	2
@@43f:
	mov	ax, [bp+4]
	out	dx, al
	cmp	al, 3Fh
	jnz	@@44e
	mov	byte ptr UartMode, 1
	jmp	short @@458
@@44e:
	mov	al, UartMode
	cmp	al, 1
	mov	ax, 0
	jz	@@45b
@@458:
	call	MpuRead
@@45b:
	popf
	pop	cx
	pop	bp
	ret	2
MpuCommand	endp

; MpuWrite(byte), dropping any input waiting; AX = FFFFh if never ready.
MpuWrite	proc	near
	push	bp
	mov	bp, sp
	push	cx
	mov	cx, 0FFFFh
@@468:
	mov	dx, MpuPort
	inc	dx
	in	al, dx
	test	al, 40h
	jz	@@485
	test	al, 80h
	jnz	@@47b
	mov	dx, MpuPort
	in	al, dx
@@47b:
	loop	@@468
	mov	ax, 0FFFFh
	pop	cx
	pop	bp
	ret	2
@@485:
	mov	ax, [bp+4]
	mov	dx, MpuPort
	out	dx, al
	pop	cx
	pop	bp
	ret	2
MpuWrite	endp

; MpuRead: AX = the next byte, or FFFFh.
MpuRead	proc	near
	push	cx
	mov	cx, 0FFFFh
@@496:
	mov	dx, MpuPort
	inc	dx
	in	al, dx
	test	al, 80h
	jz	@@4a7
	loop	@@496
	mov	ax, 0FFFFh
	pop	cx
	ret
@@4a7:
	mov	dx, MpuPort
	in	al, dx
	mov	ah, 0
	pop	cx
	ret
MpuRead	endp

; Timer: nothing to do but chain.
Timer	proc	far
	pushf
	call	dword ptr cs:OldTimer
	iret
Timer	endp


code	ends
	end	ComEntry
