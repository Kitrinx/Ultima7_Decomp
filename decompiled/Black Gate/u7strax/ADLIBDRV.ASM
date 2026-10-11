; U7STRAX.DRV record 1 (ADLIBDRV): the AdLib (OPL2) music driver, 4028 bytes, loaded at offset 0 of
; its own segment. Turbo Assembler 2.51 and TLINK /t rebuild it byte for byte.
;
; The game calls the entries in the table at the start; the far ones take Pascal arguments.
; Voices 0-8 are the OPL2 channels. In rhythm mode voices 6-10 are the rhythm sounds (bass drum,
; snare, tom, cymbal, hi-hat) and only 0-5 are melodic. Melodic voices sit on two circular lists
; through VoiceNext: free voices after node 11 and busy ones after node 12, oldest first.

	LOCALS

; A timbre in the bank: 48 bytes, addressed by its segment at TimbreOffset.
T_MOD           equ 00h                 ; modulator 20h, 40h, 60h, 80h, E0h values
T_CAR           equ 05h                 ; carrier, the same five
T_FEEDBACK      equ 0Ah                 ; C0h value
T_RHYTHM        equ 0Bh                 ; rhythm voice 6-10, or 0 for melodic
T_CAR_TOUCH     equ 0Ch                 ; carrier level's velocity scaling, 0 none
T_MOD_TOUCH     equ 0Dh                 ; modulator's
T_BEND_RANGE    equ 0Eh
T_WHEEL         equ 0Fh                 ; vibrato depth added at full mod wheel
T_VIB_RATE      equ 10h
T_VIB_DEPTH     equ 11h
T_SLIDE         equ 12h                 ; pitch slide start, then rate and target words per stage
T_TUNE          equ 24h                 ; detune, in 256ths of a semitone
T_LAYER         equ 26h                 ; nonzero: the next timbre plays too
T_KEY_SHIFT     equ 27h                 ; note distance from middle C is shifted right by this
T_PROGRAM       equ 2Fh

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
	dw	offset ReleaseVoice, offset ClaimVoice

; What Describe returns; the game fills in the hooks.
DriverInfo	db	0
	db	1                               ; keep the timbre bank
OldTimer	dd	0
	db	4 dup (0)
	dd	0                               ; hook an interrupt
	dd	0                               ; delay
	dw	30                              ; first channel free for sound effects

TimbreOffset	dw	0                   ; the first timbre's offset
TimbreSegment	dw	0
TimbreCount	db	0
OplPort	dw	388h
VoiceCount	db	9                     ; melodic voices: 9, or 6 in rhythm mode
RhythmBits	db	0                     ; register BDh
VoiceNote	db	11 dup (0FFh)
VoicePitch	dw	11 dup (0)            ; the note as a pitch, before bend, slide and vibrato
VoiceTimbre	dw	11 dup (0FFFFh)      ; segment, FFFFh none
VoiceState	db	11 dup (1)            ; 0 released, 1 idle, 2 up held (slide stages)
VoiceNext	db	1, 2, 3, 4, 5, 6, 7, 8, 11, 0FFh, 0FFh
FreeVoices	db	0
BusyVoices	db	12
VoiceChannel	db	11 dup (0FFh)       ; from 0
ChannelTimbre	dw	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 5
	dw	16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31
VoiceSlide	dw	11 dup (0)
ChannelBend	dw	32 dup (0)
VoiceVibrato	dw	11 dup (0)
VoicePhase	db	11 dup (0)
ChannelVibRate	db	32 dup (0)
ChannelVibDepth	dw	32 dup (0)
ChannelVolume	dw	32 dup (100h)      ; volume + 80h
RhythmBit	db	10h, 08h, 04h, 02h, 01h ; BDh bits of rhythm voices 6-10
CarrierSlot	db	03h, 04h, 05h, 0Bh, 0Ch, 0Dh, 13h, 14h, 15h
	db	03h, 04h, 05h, 0Bh, 0Ch, 0Dh, 13h, 14h, 12h, 15h, 11h
ModulatorSlot	db	00h, 01h, 02h, 08h, 09h, 0Ah, 10h, 11h, 12h
	db	00h, 01h, 02h, 08h, 09h, 0Ah, 10h, 14h, 12h, 15h, 11h
FNumber	dw	01E5h, 0202h, 0220h, 0241h, 0263h, 0287h, 02AEh, 02D7h, 0302h, 0330h
	dw	0360h, 0393h, 03CAh
SlideStage	db	18h, 0, 12h, 14h, 16h, 0, 0, 0 ; by voice state: half the stage offset + 8

; Waits CX ticks of the 1.19 MHz timer, reading channel 0 as it counts down.
WaitTicks	proc	near
	push	si
	push	bx
	xor	si, si
	mov	al, 6
	out	43h, al
	mov	al, 36h
	in	al, 40h
	mov	bl, al
	in	al, 40h
	mov	bh, al
@@249:
	mov	dx, bx
	mov	al, 6
	out	43h, al
	mov	al, 36h
	in	al, 40h
	mov	bl, al
	in	al, 40h
	mov	bh, al
	sub	dx, bx
	jnc	@@261
	add	dx, 4DAEh
@@261:
	add	si, dx
	cmp	si, cx
	jc	@@249
	pop	bx
	pop	si
	ret
WaitTicks	endp

; Writes AH to OPL register AL, with the waits the chip needs after each write.
OplWrite	proc	near
	pushf
	cli
	mov	dx, OplPort
	out	dx, al
	mov	cx, 0Ah
	call	WaitTicks
	mov	dx, OplPort
	inc	dx
	mov	al, ah
	out	dx, al
	mov	cx, 37h
	call	WaitTicks
	popf
	ret
OplWrite	endp

; Returns the driver information in DX:AX.
Describe	proc	far
	mov	ax, offset DriverInfo
	mov	dx, cs
	retf
Describe	endp

; Init(timbres): wave select on, rhythm and CSM off. Keeps the timbre bank, a count byte
; then the timbres, and fills ChannelTimbre with AX, which holds the count here, so no
; channel has a timbre yet. Returns the driver information (nonzero: success).
Init	proc	far
	push	bp
	mov	bp, sp
	push	si
	push	di
	push	ds
	mov	ax, cs
	mov	ds, ax
	mov	es, ax
	mov	byte ptr cs:VoiceCount, 9
	mov	ax, 2001h
	call	OplWrite
	mov	ax, 0BDh
	call	OplWrite
	mov	ax, 8
	call	OplWrite
	mov	bx, [bp+6]
	mov	ds, word ptr [bp+8]
	mov	al, [bx]
	mov	cs:TimbreCount, al
	inc	bx
	mov	word ptr cs:TimbreSegment, ds
	mov	cs:TimbreOffset, bx
	mov	cx, 20h
        mov     di, offset ChannelTimbre ; AX = timbre count
	rep stosw
	mov	ax, offset DriverInfo
	mov	dx, cs
	pop	ds
	pop	di
	pop	si
	pop	bp
	retf	4
Init	endp

; SetPort(port)
SetPort	proc	far
	push	bp
	mov	bp, sp
	mov	ax, [bp+6]
	mov	cs:OplPort, ax
	pop	bp
	retf	2
SetPort	endp

; Sysex(address, length, data): nothing to do.
Sysex	proc	far
	push	bp
	mov	bp, sp
	pop	bp
	retf	0Ah
Sysex	endp

; ReleaseVoice(voice): lends melodic voice 0-8 to the sound effects. Silences it and
; takes it off the voice lists.
ReleaseVoice	proc	far
	push	bp
	mov	bp, sp
	push	ds
	mov	ax, cs
	mov	ds, ax
	mov	si, [bp+6]
	cmp	si, 0
	jl	@@33e
	cmp	si, 8
	jg	@@33e
	mov	al, VoiceNext[si]
	xor	ah, ah
	cmp	ax, si
	jz	@@33e
	pushf
	cli
	mov	byte ptr VoiceChannel[si], 0FFh
	mov	byte ptr VoiceState[si], 1
	shl	si, 1
	mov	word ptr VoiceTimbre[si], 0FFFFh
	shr	si, 1
	mov	ax, si
	add	al, 0A0h
	xor	ah, ah
	call	OplWrite
	mov	ax, si
	add	al, 0B0h
	xor	ah, ah
	call	OplWrite
	push	si
	push	cs
	call	near ptr Unlink
	popf
@@33e:
	pop	ds
	pop	bp
	retf	2
ReleaseVoice	endp

; ClaimVoice(voice): takes a lent voice back, at the end of the free list.
ClaimVoice	proc	far
	push	bp
	mov	bp, sp
	push	ds
	mov	ax, cs
	mov	ds, ax
	mov	si, [bp+6]
	cmp	si, 0
	jl	@@36b
	cmp	si, 8
	jg	@@36b
	mov	al, VoiceNext[si]
	xor	ah, ah
	cmp	ax, si
	jnz	@@36b
	push	si
	mov	ax, 0Bh
	push	ax
	push	cs
	call	near ptr LinkBefore
@@36b:
	pop	ds
	pop	bp
	retf	2
ClaimVoice	endp

; Shutdown: rhythm off and every voice keyed off.
Shutdown	proc	far
	push	bp
	mov	bp, sp
	push	si
	push	ds
	mov	ax, cs
	mov	ds, ax
	mov	byte ptr VoiceCount, 0
	mov	ax, 0BDh
	call	OplWrite
	xor	si, si
	jmp	short @@39b
@@388:
	mov	ax, si
	add	al, 0A0h
	xor	ah, ah
	call	OplWrite
	mov	ax, si
	add	al, 0B0h
	xor	ah, ah
	call	OplWrite
	inc	si
@@39b:
	cmp	si, 9
	jl	@@388
	pop	ds
	pop	si
	pop	bp
	retf
Shutdown	endp

; CarrierOf(voice): its carrier slot. In rhythm mode the table starts 9 entries on.
CarrierOf	proc	far
	push	bp
	mov	bp, sp
	push	si
	mov	si, [bp+6]
	cmp	byte ptr VoiceCount, 9
	jnc	@@3b5
	add	si, 9
@@3b5:
	mov	al, CarrierSlot[si]
	xor	ah, ah
	pop	si
	pop	bp
	retf	2
CarrierOf	endp

; ModulatorOf(voice): its modulator slot, or the single slot of a rhythm sound.
ModulatorOf	proc	far
	push	bp
	mov	bp, sp
	push	si
	mov	si, [bp+6]
	cmp	byte ptr VoiceCount, 9
	jnc	@@3d1
	add	si, 9
@@3d1:
	mov	al, ModulatorSlot[si]
	xor	ah, ah
	pop	si
	pop	bp
	retf	2
ModulatorOf	endp

; LoadTimbre(voice, timbre): writes the timbre's operators to the voice. The rhythm
; sounds that use one operator (rhythm voice 7 up) leave the carrier and feedback alone.
LoadTimbre	proc	far
	push	bp
	mov	bp, sp
	dec	sp
	dec	sp
	push	si
	push	di
	mov	si, [bp+6]
	push	word ptr [bp+8]
	push	cs
	call	near ptr CarrierOf
	mov	[bp-2], ax
	push	word ptr [bp+8]
	push	cs
	call	near ptr ModulatorOf
	mov	di, ax
	add	ax, 20h
	push	ax
	mov	es, si
	mov	bx, TimbreOffset
	mov	al, es:[bx]
	mov	ah, al
	pop	cx
	mov	al, cl
	call	OplWrite
	mov	ax, di
	add	ax, 40h
	push	ax
	inc	bx
	mov	al, es:[bx]
	mov	ah, al
	pop	cx
	mov	al, cl
	call	OplWrite
	mov	ax, di
	add	ax, 60h
	push	ax
	inc	bx
	mov	al, es:[bx]
	mov	ah, al
	pop	cx
	mov	al, cl
	call	OplWrite
	mov	ax, di
	add	ax, 80h
	push	ax
	inc	bx
	mov	al, es:[bx]
	mov	ah, al
	pop	cx
	mov	al, cl
	call	OplWrite
	mov	ax, di
	add	ax, 0E0h
	push	ax
	inc	bx
	mov	al, es:[bx]
	mov	ah, al
	pop	cx
	mov	al, cl
	call	OplWrite
	cmp	byte ptr VoiceCount, 9
	jz	@@46d
	mov	bx, TimbreOffset
	add	bx, T_RHYTHM
	cmp	byte ptr es:[bx], 7
	jc	@@46d
	jmp	@@4e5
@@46d:
	mov	ax, [bp-2]
	add	ax, 20h
	push	ax
	mov	bx, TimbreOffset
	add	bx, T_CAR
	mov	al, es:[bx]
	mov	ah, al
	pop	cx
	mov	al, cl
	call	OplWrite
	mov	ax, [bp-2]
	add	ax, 40h
	push	ax
	inc	bx
	mov	al, es:[bx]
	mov	ah, al
	pop	cx
	mov	al, cl
	call	OplWrite
	mov	ax, [bp-2]
	add	ax, 60h
	push	ax
	inc	bx
	mov	al, es:[bx]
	mov	ah, al
	pop	cx
	mov	al, cl
	call	OplWrite
	mov	ax, [bp-2]
	add	ax, 80h
	push	ax
	inc	bx
	mov	al, es:[bx]
	mov	ah, al
	pop	cx
	mov	al, cl
	call	OplWrite
	mov	ax, [bp-2]
	add	ax, 0E0h
	push	ax
	inc	bx
	mov	al, es:[bx]
	mov	ah, al
	pop	cx
	mov	al, cl
	call	OplWrite
	mov	ax, [bp+8]
	add	ax, 0C0h
	push	ax
	inc	bx
	mov	al, es:[bx]
	mov	ah, al
	pop	cx
	mov	al, cl
	call	OplWrite
@@4e5:
	pop	di
	pop	si
	mov	sp, bp
	pop	bp
	retf	4
LoadTimbre	endp

; Unlink(voice): takes the voice out of its list and points it at itself.
Unlink	proc	far
	push	bp
	mov	bp, sp
	push	si
	mov	dx, [bp+6]
	xor	si, si
	jmp	short @@513
@@4f8:
	mov	al, VoiceNext[si]
	xor	ah, ah
	cmp	ax, dx
	jnz	@@512
	mov	bx, dx
	mov	al, VoiceNext[bx]
	mov	VoiceNext[si], al
	mov	al, dl
	mov	VoiceNext[bx], al
@@512:
	inc	si
@@513:
	cmp	si, 0Dh
	jl	@@4f8
	pop	si
	pop	bp
	retf	2
Unlink	endp

; LinkBefore(voice, node): puts the voice in front of node. The list heads are nodes
; 11 (free) and 12 (busy), so in front of a head is the end of its list.
LinkBefore	proc	far
	push	bp
	mov	bp, sp
	push	si
	xor	si, si
	jmp	short @@544
@@525:
	mov	al, VoiceNext[si]
	xor	ah, ah
	cmp	ax, [bp+6]
	jnz	@@543
	mov	al, [bp+8]
	mov	VoiceNext[si], al
	mov	bx, [bp+8]
	mov	al, [bp+6]
	mov	VoiceNext[bx], al
	jmp	short @@549
@@543:
	inc	si
@@544:
	cmp	si, 0Dh
	jl	@@525
@@549:
	pop	si
	pop	bp
	retf	4
LinkBefore	endp

; AllocateVoice(timbre, velocity, note, channel): the voice for a note, or -1. In rhythm
; mode a rhythm sound has its own voice and is keyed off first. A note on takes the
; oldest free voice, or steals the oldest busy one when the timbre is the channel's own;
; a note off finds the voice playing it and frees it. A voice that last played another
; timbre or channel is loaded with this one.
AllocateVoice	proc	far
	push	bp
	mov	bp, sp
	push	si
	mov	si, 0FFFFh
	cmp	byte ptr VoiceCount, 9
	jc	@@55f
	jmp	@@594
@@55f:
	mov	ax, [bp+6]
	mov	es, ax
	mov	bx, TimbreOffset
	add	bx, T_RHYTHM
	cmp	byte ptr es:[bx], 0
	jz	@@594
	mov	al, es:[bx]
	xor	ah, ah
	mov	si, ax
	mov	al, RhythmBit-6[si]
	not	al
	mov	dl, RhythmBits
	and	dl, al
	mov	RhythmBits, dl
	mov	al, 0BDh
	mov	ah, RhythmBits
	call	OplWrite
	jmp	@@65f
@@594:
	cmp	word ptr [bp+8], 0
	jnz	@@59d
	jmp	@@606
@@59d:
	cmp	byte ptr FreeVoices, 0Bh
	jz	@@5bd
	mov	al, FreeVoices
	xor	ah, ah
	mov	si, ax
	mov	al, VoiceNext[si]
	mov	FreeVoices, al
	push	si
	mov	ax, 0Ch
	push	ax
	push	cs
	call	near ptr LinkBefore
	jmp	short @@5f4
@@5bd:
	mov	bx, [bp+0Ch]
	shl	bx, 1
	mov	ax, ChannelTimbre[bx]
	cmp	ax, [bp+6]
	jnz	@@5f4
	mov	al, BusyVoices
	xor	ah, ah
	mov	si, ax
	mov	al, VoiceNext[si]
	mov	BusyVoices, al
	push	si
	mov	ax, 0Ch
	push	ax
	push	cs
	call	near ptr LinkBefore
	mov	ax, si
	add	al, 0A0h
	xor	ah, ah
	call	OplWrite
	mov	ax, si
	add	al, 0B0h
	xor	ah, ah
	call	OplWrite
@@5f4:
	or	si, si
	jl	@@65f
	mov	byte ptr VoiceState[si], 2
	mov	al, [bp+0Ah]
	mov	VoiceNote[si], al
	jmp	short @@65f
@@606:
	xor	si, si
	jmp	short @@64a
@@60a:
	cmp	byte ptr VoiceState[si], 1
	jna	@@649
	mov	al, VoiceNote[si]
	xor	ah, ah
	cmp	ax, [bp+0Ah]
	jnz	@@649
	mov	al, VoiceChannel[si]
	xor	ah, ah
	cmp	ax, [bp+0Ch]
	jnz	@@649
	shl	si, 1
	mov	ax, VoiceTimbre[si]
	shr	si, 1
	cmp	ax, [bp+6]
	jnz	@@649
	mov	byte ptr VoiceState[si], 0
	push	si
	push	cs
	call	near ptr Unlink
	push	si
	mov	ax, 0Bh
	push	ax
	push	cs
	call	near ptr LinkBefore
	jmp	short @@653
@@649:
	inc	si
@@64a:
	mov	al, VoiceCount
	xor	ah, ah
	cmp	ax, si
	jg	@@60a
@@653:
	mov	al, VoiceCount
	xor	ah, ah
	cmp	ax, si
	jg	@@65f
	mov	si, 0FFFFh
@@65f:
	or	si, si
	jl	@@695
	mov	al, VoiceChannel[si]
	xor	ah, ah
	cmp	ax, [bp+0Ch]
	jnz	@@67b
	shl	si, 1
	mov	ax, VoiceTimbre[si]
	shr	si, 1
	cmp	ax, [bp+6]
	jz	@@695
@@67b:
	push	si
	push	word ptr [bp+6]
	push	cs
	call	near ptr LoadTimbre
	mov	al, [bp+0Ch]
	mov	VoiceChannel[si], al
	mov	ax, [bp+6]
	shl	si, 1
	mov	VoiceTimbre[si], ax
	shr	si, 1
@@695:
	mov	ax, si
	pop	si
	pop	bp
	retf	8
AllocateVoice	endp

; PitchToFrequency(pitch): pitch in 256ths of a semitone, 60 is middle C. Returns the
; block in bits 10-12 and the F-number below, interpolated between table entries. The
; upper entry is found from (semitone - 18) mod 12, which is negative below semitone 18.
PitchToFrequency	proc	far
	push	bp
	mov	bp, sp
	dec	sp
	dec	sp
	push	si
	push	di
	mov	ax, [bp+6]
	mov	cl, 8
	sar	ax, cl
	mov	[bp-2], ax
	add	ax, 6
	mov	bx, 0Ch
	cwd
	idiv	bx
	sub	ax, 2
	mov	si, ax
	cmp	ax, 7
	jng	@@6c3
	mov	si, 7
@@6c3:
	or	si, si
	jnl	@@6c9
	xor	si, si
@@6c9:
	mov	ax, [bp-2]
	add	ax, 6
	mov	bx, 0Ch
	cwd
	idiv	bx
	shl	dx, 1
	mov	bx, dx
	mov	di, FNumber[bx]
	test	word ptr [bp+6], 0FFh
	jz	@@709
	mov	ax, [bp-2]
	add	ax, 0FFEEh
	mov	bx, 0Ch
	cwd
	idiv	bx
	shl	dx, 1
	mov	bx, dx
	mov	ax, FNumber+2[bx]
	sub	ax, di
	mov	dx, [bp+6]
	and	dx, 0FFh
	imul	dx
	mov	cl, 8
	sar	ax, cl
	add	di, ax
@@709:
	mov	ax, si
	mov	cl, 0Ah
	shl	ax, cl
	push	ax
	mov	ax, di
	pop	dx
	add	ax, dx
	pop	di
	pop	si
	mov	sp, bp
	pop	bp
	retf	2
PitchToFrequency	endp

; FindTimbre(AL = program): DX = segment of the timbre with that program, else the
; first. Timbres are three paragraphs apart and all sit at TimbreOffset.
FindTimbre	proc	near
	mov	dx, cs:TimbreSegment
	mov	ds, dx
	mov	si, cs:TimbreOffset
	add	si, T_PROGRAM
	mov	cl, cs:TimbreCount
	xor	ch, ch
@@733:
	cmp	al, [si]
	jz	@@743
	add	dx, 3
	mov	ds, dx
	loop	@@733
	mov	dx, cs:TimbreSegment
@@743:
	ret
FindTimbre	endp

; Note(channel, note, velocity): channel from 1, velocity 0 is note off. Plays every
; timbre of the channel's chain: a timbre with T_LAYER set goes on to the next one.
Note	proc	far
	push	bp
	mov	bp, sp
	sub	sp, 0Eh
	push	si
	push	di
	push	ds
	mov	ax, cs
	mov	ds, ax
	dec	word ptr [bp+0Ah]
	mov	bx, [bp+0Ah]
	shl	bx, 1
	mov	ax, ChannelTimbre[bx]
	jmp	@@a86
@@760:
	mov	ax, [bp-8]
	mov	[bp-0Ch], ax
	mov	ax, TimbreOffset
	mov	[bp-0Eh], ax
	push	word ptr [bp+0Ah]
	push	word ptr [bp+8]
	push	word ptr [bp+6]
	push	word ptr [bp-8]
	push	cs
	call	near ptr AllocateVoice
	mov	si, ax
	cmp	ax, 8
	jng	@@78a
	mov	ax, 11h
	sub	ax, si
	jmp	short @@78c
@@78a:
	mov	ax, si
@@78c:
	mov	[bp-4], ax
	or	si, si
	jnl	@@796
	jmp	@@a66
@@796:
	mov	dx, [bp-0Ch]
	mov	ax, [bp-0Eh]
	xor	cx, cx
	add	ax, T_TUNE
	mov	bx, ax
	mov	es, dx
	mov	al, es:[bx]
	xor	ah, ah
	mov	dx, [bp-0Ch]
	push	ax
	mov	ax, [bp-0Eh]
	xor	cx, cx
	add	ax, T_TUNE+1
	mov	bx, ax
	mov	es, dx
	mov	al, es:[bx]
	xor	ah, ah
	mov	cl, 8
	shl	ax, cl
	pop	dx
	add	dx, ax
	mov	[bp-0Ah], dx
	cmp	word ptr [bp+6], 0
	jz	@@815
	mov	bx, si
	shl	bx, 1
	xor	ax, ax
	mov	VoiceVibrato[bx], ax
	mov	VoicePhase[si], al
	mov	dx, [bp-0Ch]
	mov	ax, [bp-0Eh]
	xor	cx, cx
	add	ax, T_SLIDE
	mov	bx, ax
	mov	es, dx
	mov	al, es:[bx]
	xor	ah, ah
	mov	dx, [bp-0Ch]
	push	ax
	mov	ax, [bp-0Eh]
	xor	cx, cx
	add	ax, T_SLIDE+1
	mov	bx, ax
	mov	es, dx
	mov	al, es:[bx]
	xor	ah, ah
	mov	cl, 8
	shl	ax, cl
	pop	dx
	add	dx, ax
	mov	bx, si
	shl	bx, 1
	mov	VoiceSlide[bx], dx
@@815:
	mov	ah, [bp+8]
	sub	ah, 3Ch
	xor	al, al
	mov	es, word ptr [bp-0Ch]
	mov	bx, [bp-0Eh]
	add	bx, T_KEY_SHIFT
	mov	cl, es:[bx]
	or	cl, cl
	jnl	@@831
	neg	ax
	not	cl
@@831:
	sar	ax, cl
	add	ah, 3Ch
	mov	bx, si
	shl	bx, 1
	mov	VoicePitch[bx], ax
	mov	bx, [bp+0Ah]
	shl	bx, 1
	add	ax, ChannelBend[bx]
	mov	bx, si
	shl	bx, 1
	add	ax, VoiceSlide[bx]
	mov	bx, si
	shl	bx, 1
	add	ax, VoiceVibrato[bx]
	add	ax, [bp-0Ah]
	push	ax
	push	cs
	call	near ptr PitchToFrequency
	mov	[bp-2], ax
	cmp	word ptr [bp+6], 0
	jnz	@@8a2
	mov	al, VoiceCount
	xor	ah, ah
	cmp	ax, si
	jnl	@@889
	cmp	si, 6
	jng	@@889
	mov	al, RhythmBit-6[si]
	not	al
	mov	dl, RhythmBits
	and	dl, al
	mov	RhythmBits, dl
	jmp	@@a48
@@889:
	mov	ax, [bp-4]
	add	al, 0A0h
	mov	ah, [bp-2]
	call	OplWrite
	mov	ax, [bp-4]
	add	al, 0B0h
	mov	ah, [bp-1]
	call	OplWrite
	jmp	@@a48
@@8a2:
	mov	dx, [bp-0Ch]
	mov	ax, [bp-0Eh]
	xor	cx, cx
	add	ax, T_CAR+1
	mov	bx, ax
	mov	es, dx
	mov	al, es:[bx]
	xor	ah, ah
	mov	[bp-6], ax
	mov	dx, [bp-0Ch]
	mov	ax, [bp-0Eh]
	xor	cx, cx
	add	ax, T_CAR_TOUCH
	mov	bx, ax
	mov	es, dx
	cmp	byte ptr es:[bx], 0
	jnz	@@8db
	mov	bx, [bp+0Ah]
	shl	bx, 1
	cmp	word ptr ChannelVolume[bx], 100h
	jnl	@@93e
@@8db:
	mov	dx, [bp-0Ch]
	mov	ax, [bp-0Eh]
	xor	cx, cx
	add	ax, T_CAR_TOUCH
	mov	cl, 7
	mov	bx, ax
	mov	es, dx
	sub	cl, es:[bx]
	mov	ax, 3Fh
	sub	ax, [bp+6]
	sar	ax, cl
	mov	dx, [bp-6]
	and	dx, 3Fh
	add	ax, dx
	mov	di, ax
	mov	bx, [bp+0Ah]
	shl	bx, 1
	mov	ax, ChannelVolume[bx]
	mov	dx, 3Fh
	sub	dx, di
	imul	dx
	mov	cl, 8
	sar	ax, cl
	mov	dx, 3Fh
	sub	dx, ax
	mov	di, dx
	cmp	dx, 3Fh
	jng	@@924
	mov	di, 3Fh
@@924:
	or	di, di
	jnl	@@92a
	xor	di, di
@@92a:
	push	si
	push	cs
	call	near ptr CarrierOf
	add	al, 40h
	mov	ah, [bp-6]
	and	ah, 0C0h
	mov	dx, di
	add	ah, dl
	call	OplWrite
@@93e:
	mov	dx, [bp-0Ch]
	mov	ax, [bp-0Eh]
	xor	cx, cx
	inc	ax
	mov	bx, ax
	mov	es, dx
	mov	al, es:[bx]
	xor	ah, ah
	mov	[bp-6], ax
	mov	dx, [bp-0Ch]
	mov	ax, [bp-0Eh]
	xor	cx, cx
	add	ax, T_MOD_TOUCH
	mov	bx, ax
	mov	es, dx
	cmp	byte ptr es:[bx], 0
	jnz	@@975
	mov	bx, [bp+0Ah]
	shl	bx, 1
	cmp	word ptr ChannelVolume[bx], 100h
	jnl	@@9df
@@975:
	mov	dx, [bp-0Ch]
	mov	ax, [bp-0Eh]
	xor	cx, cx
	add	ax, T_MOD_TOUCH
	mov	cl, 7
	mov	bx, ax
	mov	es, dx
	sub	cl, es:[bx]
	mov	ax, 3Fh
	sub	ax, [bp+6]
	sar	ax, cl
	mov	dx, [bp-6]
	and	dx, 3Fh
	add	ax, dx
	mov	di, ax
        test    byte ptr es:[bx-3], 1   ; T_FEEDBACK: additive
	jz	@@9bd
	mov	bx, [bp+0Ah]
	shl	bx, 1
	mov	ax, ChannelVolume[bx]
	mov	dx, 3Fh
	sub	dx, di
	imul	dx
	mov	cl, 8
	sar	ax, cl
	mov	dx, 3Fh
	sub	dx, ax
	mov	di, dx
@@9bd:
        cmp     ax, 3Fh                 ; AX, not DI
	jng	@@9c5
	mov	di, 3Fh
@@9c5:
	or	di, di
	jnl	@@9cb
	xor	di, di
@@9cb:
	push	si
	push	cs
	call	near ptr ModulatorOf
	add	al, 40h
	mov	ah, [bp-6]
	and	ah, 0C0h
	mov	dx, di
	add	ah, dl
	call	OplWrite
@@9df:
	mov	dx, [bp-0Ch]
	mov	ax, [bp-0Eh]
	xor	cx, cx
	add	ax, T_RHYTHM
	mov	bx, ax
	mov	es, dx
	cmp	byte ptr es:[bx], 0
	jz	@@9f9
	cmp	si, 6
	jnz	@@a2b
@@9f9:
	mov	dx, [bp-0Ch]
	mov	ax, [bp-0Eh]
	xor	cx, cx
	add	ax, T_RHYTHM
	mov	bx, ax
	mov	es, dx
	cmp	byte ptr es:[bx], 0
	jnz	@@a13
	add	word ptr [bp-2], 2000h
@@a13:
	mov	ax, [bp-4]
	add	ax, 0A0h
	mov	ah, [bp-2]
	call	OplWrite
	mov	ax, [bp-4]
	add	ax, 0B0h
	mov	ah, [bp-1]
	call	OplWrite
@@a2b:
	mov	dx, [bp-0Ch]
	mov	ax, [bp-0Eh]
	xor	cx, cx
	add	ax, T_RHYTHM
	mov	bx, ax
	mov	es, dx
	cmp	byte ptr es:[bx], 0
	jz	@@a48
	mov	al, RhythmBit-6[si]
	or	RhythmBits, al
@@a48:
	mov	dx, [bp-0Ch]
	mov	ax, [bp-0Eh]
	xor	cx, cx
	add	ax, T_RHYTHM
	mov	bx, ax
	mov	es, dx
	cmp	byte ptr es:[bx], 0
	jz	@@a66
	mov	al, 0BDh
	mov	ah, RhythmBits
	call	OplWrite
@@a66:
	mov	dx, [bp-0Ch]
	mov	ax, [bp-0Eh]
	xor	cx, cx
	add	ax, T_LAYER
	mov	bx, ax
	mov	es, dx
	mov	al, es:[bx]
	xor	ah, ah
	dec	ax
	cmp	ax, 0FFFFh
	jz	@@a86
	mov	ax, [bp-8]
	add	ax, 3
@@a86:
	mov	[bp-8], ax
	cmp	word ptr [bp-8], -1
	jz	@@a92
	jmp	@@760
@@a92:
	pop	ds
	pop	di
	pop	si
	mov	sp, bp
	pop	bp
	retf	6
Note	endp

; Pitch(channel, low, high): the 14-bit bend, scaled by the timbre's bend range.
; Retunes the channel's sounding voices.
Pitch	proc	far
	push	bp
	mov	bp, sp
	sub	sp, 4
	push	si
	push	di
	push	ds
	mov	ax, cs
	mov	ds, ax
	mov	di, [bp+0Ah]
	dec	di
	shl	di, 1
	mov	ax, ChannelTimbre[di]
	shr	di, 1
	mov	es, ax
	mov	bx, TimbreOffset
	add	bx, T_BEND_RANGE
	mov	al, es:[bx]
	xor	ah, ah
	push	ax
	mov	ax, [bp+6]
	mov	cl, 7
	shl	ax, cl
	add	ax, [bp+8]
	add	ax, 0E000h
	pop	cx
	imul	cx
	mov	al, ah
	mov	ah, dl
	mov	bx, di
	shl	bx, 1
	mov	ChannelBend[bx], ax
	xor	si, si
	jmp	@@b6f
@@ae4:
	cmp	byte ptr VoiceState[si], 1
	ja	@@aee
	jmp	@@b6e
@@aee:
	mov	al, VoiceChannel[si]
	xor	ah, ah
	cmp	ax, di
	jz	@@afb
	jmp	@@b6e
@@afb:
	shl	si, 1
	cmp	word ptr VoiceTimbre[si], -1
	pushf
	shr	si, 1
	popf
	jz	@@b21
	shl	si, 1
	mov	ax, VoiceTimbre[si]
	shr	si, 1
	mov	es, ax
	mov	bx, TimbreOffset
	add	bx, T_TUNE
	mov	ax, es:[bx]
	mov	[bp-4], ax
	jmp	short @@b26
@@b21:
	mov	word ptr [bp-4], 0
@@b26:
	mov	bx, si
	shl	bx, 1
	mov	ax, VoicePitch[bx]
	mov	cl, 8
	mov	bx, di
	shl	bx, 1
	add	ax, ChannelBend[bx]
	mov	bx, si
	shl	bx, 1
	add	ax, VoiceSlide[bx]
	mov	bx, si
	shl	bx, 1
	add	ax, VoiceVibrato[bx]
	add	ax, [bp-4]
	push	ax
	push	cs
	call	near ptr PitchToFrequency
	mov	[bp-2], ax
	add	word ptr [bp-2], 2000h
	mov	ax, si
	add	ax, 0A0h
	mov	ah, [bp-2]
	call	OplWrite
	mov	ax, si
	add	ax, 0B0h
	mov	ah, [bp-1]
	call	OplWrite
@@b6e:
	inc	si
@@b6f:
	mov	al, VoiceCount
	xor	ah, ah
	cmp	ax, si
	jng	@@b7b
	jmp	@@ae4
@@b7b:
	pop	ds
	pop	di
	pop	si
	mov	sp, bp
	pop	bp
	retf	6
Pitch	endp

; Control(channel, control, value): 1 vibrato depth, 7 volume, 79h reset, 7Bh all notes
; off, which also leaves rhythm mode once no rhythm sound plays. Channel 10 passes it on
; to channels 11-30 first.
Control	proc	far
	push	bp
	mov	bp, sp
	sub	sp, 6
	push	si
	push	di
	push	ds
	mov	ax, cs
	mov	ds, ax
	mov	di, [bp+0Ah]
	cmp	di, 0Ah
	jnz	@@bae
	inc	di
@@b9a:
	push	di
	push	word ptr [bp+8]
	push	word ptr [bp+6]
	push	cs
	call	near ptr Control
	inc	di
	cmp	di, 1Eh
	jna	@@b9a
	mov	di, 0Ah
@@bae:
	dec	di
	mov	ax, [bp+8]
	cmp	ax, 1
	jz	@@bd2
	cmp	ax, 7
	jnz	@@bbf
	jmp	@@c20
@@bbf:
	cmp	ax, 7Bh
	jnz	@@bc7
	jmp	@@c5c
@@bc7:
	cmp	ax, 79h
	jnz	@@bcf
	jmp	@@c31
@@bcf:
	jmp	@@ccb
@@bd2:
	shl	di, 1
	mov	ax, ChannelTimbre[di]
	shr	di, 1
	mov	[bp-2], ax
	mov	dx, ax
	mov	ax, TimbreOffset
	mov	[bp-4], ax
	mov	ax, [bp-4]
	xor	cx, cx
	add	ax, T_WHEEL
	mov	bx, ax
	mov	es, dx
	mov	al, es:[bx]
	xor	ah, ah
	imul	word ptr [bp+6]
	mov	cl, 7
	sar	ax, cl
	mov	dx, [bp-2]
	push	ax
	mov	ax, [bp-4]
	xor	cx, cx
	add	ax, T_VIB_DEPTH
	mov	bx, ax
	mov	es, dx
	mov	al, es:[bx]
	xor	ah, ah
	pop	dx
	add	dx, ax
	mov	bx, di
	shl	bx, 1
	mov	ChannelVibDepth[bx], dx
	jmp	@@ccb
@@c20:
	mov	bx, di
	shl	bx, 1
	mov	ax, [bp+6]
	add	ax, 80h
	mov	ChannelVolume[bx], ax
	jmp	@@ccb
@@c31:
	mov	si, [bp+0Ah]
	push	si
	mov	ax, 1
	push	ax
	xor	ax, ax
	push	ax
	push	cs
	call	near ptr Control
	push	si
	mov	ax, 7
	push	ax
	mov	ax, 7Fh
	push	ax
	push	cs
	call	near ptr Control
	push	si
	xor	ax, ax
	push	ax
	mov	ax, 40h
	push	ax
	push	cs
	call	near ptr Pitch
	jmp	@@ccb
@@c5c:
	mov	word ptr [bp-6], 0
	xor	si, si
	jmp	short @@c99
@@c65:
	cmp	byte ptr VoiceState[si], 1
	jna	@@c98
	mov	al, VoiceChannel[si]
	xor	ah, ah
	cmp	ax, di
	jnz	@@c8a
	mov	ax, di
	inc	ax
	push	ax
	mov	al, VoiceNote[si]
	xor	ah, ah
	push	ax
	xor	ax, ax
	push	ax
	push	cs
	call	near ptr Note
	jmp	short @@c98
@@c8a:
	mov	al, VoiceCount
	xor	ah, ah
	cmp	ax, si
	jg	@@c98
	mov	word ptr [bp-6], 1
@@c98:
	inc	si
@@c99:
	cmp	si, 0Bh
	jl	@@c65
	cmp	word ptr [bp-6], 0
	jnz	@@ccb
	cmp	byte ptr VoiceCount, 9
	jnc	@@ccb
        mov     ax, 0BDh                ; rhythm off
	call	OplWrite
	mov	byte ptr VoiceCount, 9
	mov	byte ptr VoiceNext+6, 7
	mov	byte ptr VoiceNext+7, 8
	mov	al, FreeVoices
	mov	VoiceNext+8, al
	mov	byte ptr FreeVoices, 6
@@ccb:
	pop	ds
	pop	di
	pop	si
	mov	sp, bp
	pop	bp
	retf	6
Control	endp

; Program(channel, program): stops the channel's voices and gives it the timbre. The
; first rhythm timbre turns rhythm mode on, which takes voices 6-8 off the lists.
Program	proc	far
	push	bp
	mov	bp, sp
	push	si
	push	di
	push	ds
	mov	al, [bp+6]
	call	FindTimbre
	mov	[bp+6], dx
	mov	ax, cs
	mov	ds, ax
	dec	word ptr [bp+8]
	xor	si, si
	jmp	short @@d1c
@@cee:
	mov	al, VoiceChannel[si]
	xor	ah, ah
	cmp	ax, [bp+8]
	jnz	@@d1b
	mov	ax, [bp+8]
	inc	ax
	push	ax
	mov	al, VoiceNote[si]
	xor	ah, ah
	push	ax
	xor	ax, ax
	push	ax
	push	cs
	call	near ptr Note
	mov	byte ptr VoiceChannel[si], 0FFh
	shl	si, 1
	mov	word ptr VoiceTimbre[si], 0FFFFh
	shr	si, 1
@@d1b:
	inc	si
@@d1c:
	cmp	si, 0Bh
	jl	@@cee
	mov	bx, [bp+8]
	mov	ax, [bp+6]
	shl	bx, 1
	mov	ChannelTimbre[bx], ax
	mov	ax, [bp+6]
	mov	es, ax
	mov	bx, TimbreOffset
	add	bx, T_VIB_RATE
	mov	al, es:[bx]
	mov	bx, [bp+8]
	mov	ChannelVibRate[bx], al
	mov	ax, [bp+6]
	mov	es, ax
	mov	bx, TimbreOffset
	add	bx, T_VIB_DEPTH
	mov	al, es:[bx]
	xor	ah, ah
	mov	bx, [bp+8]
	shl	bx, 1
	mov	ChannelVibDepth[bx], ax
	mov	ax, [bp+6]
	mov	es, ax
	mov	bx, TimbreOffset
	add	bx, T_RHYTHM
	cmp	byte ptr es:[bx], 0
	jnz	@@d72
	jmp	@@de1
@@d72:
	cmp	byte ptr VoiceCount, 9
	jz	@@d7c
	jmp	@@de1
@@d7c:
	mov	ax, 0A6h
	call	OplWrite
	mov	ax, 0B6h
	call	OplWrite
	mov	ax, 0A7h
	call	OplWrite
	mov	ax, 0AB7h
	call	OplWrite
	mov	ax, 54A8h
	call	OplWrite
	mov	ax, 9B8h
	call	OplWrite
	mov	byte ptr VoiceCount, 6
	mov	si, 6
	jmp	short @@dce
@@daa:
	xor	di, di
	jmp	short @@dc3
@@dae:
	mov	al, VoiceNext[di]
	xor	ah, ah
	cmp	ax, si
	jnz	@@dc2
	mov	al, VoiceNext[si]
	mov	VoiceNext[di], al
	jmp	short @@dc8
@@dc2:
	inc	di
@@dc3:
	cmp	di, 0Dh
	jl	@@dae
@@dc8:
	mov	byte ptr VoiceNext[si], 0FFh
	inc	si
@@dce:
	cmp	si, 9
	jl	@@daa
        mov     byte ptr RhythmBits, 20h ; rhythm mode
	mov	al, 0BDh
	mov	ah, RhythmBits
	call	OplWrite
@@de1:
	pop	ds
	pop	di
	pop	si
	pop	bp
	retf	4
Program	endp

; Distance(a, b): |a - b|
Distance	proc	far
	push	bp
	mov	bp, sp
	push	si
	push	di
	mov	di, [bp+8]
	mov	si, [bp+6]
	cmp	di, si
	jng	@@dfd
	mov	ax, di
	sub	ax, si
	jmp	short @@e01
@@dfd:
	mov	ax, si
	sub	ax, di
@@e01:
	pop	di
	pop	si
	pop	bp
	retf	4
Distance	endp

; Timer: each timer tick steps every voice's pitch slide and vibrato and rewrites its
; frequency when either moved or the timbre is detuned; then the old handler runs.
Timer	proc	far
	push	ax
	push	bx
	push	cx
	push	dx
	push	es
	push	ds
	push	si
	push	di
	push	bp
	mov	bp, cs
	mov	ds, bp
	mov	bp, sp
	sub	sp, 12h
	xor	si, si
	jmp	@@f9f
@@e1e:
	mov	word ptr [bp-0Eh], 0
	mov	al, VoiceChannel[si]
	xor	ah, ah
	mov	di, ax
	mov	al, VoiceState[si]
	xor	ah, ah
	mov	bx, ax
	mov	al, SlideStage[bx]
	xor	ah, ah
	mov	[bp-8], ax
	shl	si, 1
	cmp	word ptr VoiceTimbre[si], -1
	pushf
	shr	si, 1
	popf
	jz	@@e66
	shl	si, 1
	mov	ax, VoiceTimbre[si]
	shr	si, 1
	mov	[bp-2], ax
	mov	ax, TimbreOffset
	mov	[bp-4], ax
	les	bx, dword ptr [bp-4]
	mov	ax, es:[bx+T_TUNE]
	mov	[bp-10h], ax
	jmp	short @@e77
@@e66:
	mov	ax, TimbreOffset
	mov	[bp-4], ax
	mov	ax, TimbreSegment
	mov	[bp-2], ax
	mov	word ptr [bp-10h], 0
@@e77:
	cmp	word ptr [bp-8], 0
	jz	@@ef1
	mov	ax, [bp-8]
	shl	ax, 1
	les	bx, dword ptr [bp-4]
	add	bx, ax
        sub     bx, 10h                 ; stage rate, then target
	mov	ax, es:[bx]
	mov	[bp-0Ah], ax
	mov	ax, [bp-8]
	inc	ax
	shl	ax, 1
	mov	bx, [bp-4]
	add	bx, ax
	sub	bx, 10h
	mov	ax, es:[bx]
	mov	[bp-0Ch], ax
	push	ax
	mov	bx, si
	shl	bx, 1
	push	word ptr VoiceSlide[bx]
	push	cs
	call	near ptr Distance
	cmp	ax, [bp-0Ah]
	jnc	@@ec7
	mov	bx, si
	shl	bx, 1
	mov	ax, [bp-0Ch]
	mov	VoiceSlide[bx], ax
	inc	byte ptr VoiceState[si]
	jmp	short @@eec
@@ec7:
	mov	bx, si
	shl	bx, 1
	mov	ax, VoiceSlide[bx]
	cmp	ax, [bp-0Ch]
	jnl	@@ee1
	mov	bx, si
	shl	bx, 1
	mov	ax, [bp-0Ah]
	add	VoiceSlide[bx], ax
	jmp	short @@eec
@@ee1:
	mov	bx, si
	shl	bx, 1
	mov	ax, [bp-0Ah]
	sub	VoiceSlide[bx], ax
@@eec:
	mov	word ptr [bp-0Eh], 1
@@ef1:
	cmp	di, 20h
	jnl	@@f43
	cmp	byte ptr ChannelVibRate[di], 0
	jz	@@f43
	mov	al, VoicePhase[si]
	add	al, ChannelVibRate[di]
	mov	VoicePhase[si], al
	mov	al, VoicePhase[si]
	mov	[bp-11h], al
	cmp	al, 3Fh
	jg	@@f18
	cmp	al, 0C0h
	jnl	@@f20
@@f18:
	mov	al, 80h
	sub	al, [bp-11h]
	mov	[bp-11h], al
@@f20:
	mov	bx, di
	shl	bx, 1
	mov	ax, ChannelVibDepth[bx]
	push	ax
	mov	al, [bp-11h]
	cbw
	mov	dx, ax
	pop	ax
	imul	dx
	mov	cl, 4
	sar	ax, cl
	mov	bx, si
	shl	bx, 1
	mov	VoiceVibrato[bx], ax
	mov	word ptr [bp-0Eh], 1
@@f43:
	cmp	word ptr [bp-0Eh], 0
	jnz	@@f4f
	cmp	word ptr [bp-10h], 0
	jz	@@f9e
@@f4f:
	mov	bx, si
	shl	bx, 1
	mov	ax, VoicePitch[bx]
	mov	cl, 8
	mov	bx, di
	shl	bx, 1
	add	ax, ChannelBend[bx]
	mov	bx, si
	shl	bx, 1
	add	ax, VoiceSlide[bx]
	mov	bx, si
	shl	bx, 1
	add	ax, VoiceVibrato[bx]
	add	ax, [bp-10h]
	push	ax
	push	cs
	call	near ptr PitchToFrequency
	mov	[bp-6], ax
	cmp	byte ptr VoiceState[si], 1
	jna	@@f88
	add	word ptr [bp-6], 2000h
@@f88:
	mov	ax, si
	add	ax, 0A0h
	mov	ah, [bp-6]
	call	OplWrite
	mov	ax, si
	add	ax, 0B0h
	mov	ah, [bp-5]
	call	OplWrite
@@f9e:
	inc	si
@@f9f:
	mov	al, VoiceCount
	xor	ah, ah
	cmp	ax, si
	jng	@@fab
	jmp	@@e1e
@@fab:
	pushf
	call	dword ptr OldTimer
	mov	sp, bp
	pop	bp
	pop	di
	pop	si
	pop	ds
	pop	es
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	iret
Timer	endp


code	ends
	end	ComEntry
