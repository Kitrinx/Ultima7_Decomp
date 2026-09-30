; Black Gate U7.EXE, resident segment 118 (file offsets 0x03a5d4 to 0x03b9f9, 5157 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.
; Folder inferred from compiler flags and link order.

	.MODEL  MEDIUM
	LOCALS

	PUBLIC  _StartAdlibSound, _TickAdlibSounds, _ResetAdlib, _StopAdlibVoice
	PUBLIC  _SetAdlibVolume, _GetAdlibVoiceState, _StartAdlibBankSound
	PUBLIC  _YieldAdlibChannels, _ReclaimAdlibChannels

NVOICES         EQU 16                  ; sounds that can play at once
NCHANNELS       EQU 9                   ; OPL2 channels; as a voice's channel, none
HOLDING         EQU 2                   ; voice states; a free voice is 0
RELEASING       EQU 1

ADLIB_PORT      EQU 388h

; OPL2 registers for the whole chip
OPL_TEST        EQU 01h                 ; waveform select enable
OPL_CSM         EQU 08h                 ; CSM and note select
OPL_RHYTHM      EQU 0BDh

; OPL2 operator registers; add the operator's offset
OPL_MULT        EQU 20h                 ; tremolo, vibrato, sustain, KSR, multiplier
OPL_LEVEL       EQU 40h                 ; key scaling and output level
OPL_AD          EQU 60h                 ; attack and decay
OPL_SR          EQU 80h                 ; sustain and release
OPL_WAVE        EQU 0E0h                ; waveform

; OPL2 channel registers; add the channel
OPL_FNUM        EQU 0A0h                ; frequency, low eight bits
OPL_KEYON       EQU 0B0h                ; key on, block, frequency high bits
OPL_FEEDBACK    EQU 0C0h                ; feedback and connection

; A value driven by a script of (ticks, step) word pairs.
ENVELOPE    STRUC
env_script  dw  ?                       ; the next pair
env_count   dw  ?                       ; ticks left in this one
env_value   dw  ?
env_step    dw  ?                       ; added to the value each tick
ENVELOPE    ENDS

; One sound parameter in a sound's header.
SNDPART     STRUC
part_start      dw  ?                   ; the value it starts at
part_attack     dw  ?                   ; its script while the note holds
part_release    dw  ?                   ; and once the note is released
SNDPART     ENDS

; A sound fills its own segment and opens with this header. The
; parameters are SNDPARTs; the scripts follow the header.
SNDHEAD     STRUC
snd_size            dw  ?               ; bytes, scripts included
snd_hold            dw  ?               ; ticks the note holds
snd_pitch           dw  3 dup (?)
snd_level1          dw  3 dup (?)
snd_level2          dw  3 dup (?)
snd_priority        dw  3 dup (?)
snd_feedback        dw  3 dup (?)
snd_mult1           dw  3 dup (?)
snd_mult2           dw  3 dup (?)
snd_wave            dw  3 dup (?)
snd_adsr            dw  2 dup (?)       ; both operators' AD and SR bytes, held
snd_release_adsr    dw  2 dup (?)       ; and released
SNDHEAD     ENDS

; A playing sound. The parameters are ENVELOPEs.
VOICEREC    STRUC
v_state         dw  ?                   ; 0 when free
v_hold          dw  ?                   ; ticks left before release
v_unused        dw  ?
v_sound         dw  ?                   ; segment of its sound
v_pitch         dw  4 dup (?)
v_level1        dw  4 dup (?)
v_level2        dw  4 dup (?)
v_feedback      dw  4 dup (?)
v_priority      dw  4 dup (?)
v_mult1         dw  4 dup (?)
v_mult2         dw  4 dup (?)
v_wave          dw  4 dup (?)           ; op1's in the high byte, op2's in the low
v_keyon         dw  ?                   ; key-on and block bits, in the high byte
v_ksl           dw  ?                   ; key scaling bits, op1 high, op2 low
v_mult_flags    dw  ?                   ; register 20h flag bits, op1 high, op2 low
v_connect       dw  ?                   ; connection bit for register C0h
v_channel       dw  ?                   ; NCHANNELS while it has none
v_volume        dw  ?                   ; its row of volumeTable
VOICEREC    ENDS

	.DATA
	EXTRN   _DriverReleaseChannelEntry:DWORD ; called with each channel handed back to us
	EXTRN   _DriverClaimChannelEntry:DWORD ; called with each channel we lend out

	.CODE
	ASSUME  DS:@code

; Write the voice's current parameters to its channel, if it has one,
; then step the envelopes that run the same way held or released.
UpdateVoice    PROC NEAR
	mov     al, channel
	cmp     al, NCHANNELS
	jne     @@write
	jmp     @@step
@@write:
	mov     bx, [si].v_ksl
	mov     ax, ticks
	rcr     ax, 1
	mov     ax, [si].v_level1.env_value
	not     ax
	shr     ax, 1
	shr     ax, 1
	or      ah, bh
	mov     al, OPL_LEVEL
	add     al, op1
	call    OplWrite
; op2's level is scaled by the voice's volume row
	mov     ax, [si].v_level2.env_value
	mov     di, [si].v_volume
	shr     ax, 1
	shr     ax, 1
	mov     al, ah
	xor     ah, ah
	add     di, ax
	mov     ah, [di]
	or      ah, bl
	mov     al, OPL_LEVEL
	add     al, op2
	call    OplWrite
	mov     ax, [si].v_feedback.env_value
	shr     ax, 1
	shr     ax, 1
	shr     ax, 1
	shr     ax, 1
	and     ah, 0Eh
	or      ax, [si].v_connect
	mov     al, OPL_FEEDBACK
	add     al, channel
	call    OplWrite
	mov     bx, [si].v_mult_flags
	mov     ax, [si].v_mult1.env_value
	shr     ax, 1
	shr     ax, 1
	shr     ax, 1
	shr     ax, 1
	or      ah, bh
	mov     al, OPL_MULT
	add     al, op1
	call    OplWrite
	mov     ax, [si].v_mult2.env_value
	shr     ax, 1
	shr     ax, 1
	shr     ax, 1
	shr     ax, 1
	or      ah, bl
	mov     al, OPL_MULT
	add     al, op2
	call    OplWrite
	mov     ax, [si].v_wave.env_value
	mov     al, OPL_WAVE
	add     al, op1
	call    OplWrite
	mov     ax, [si].v_wave.env_value
	mov     ah, al
	mov     al, OPL_WAVE
	add     al, op2
	call    OplWrite
; the frequency is the pitch's top ten bits
	mov     ax, [si].v_pitch.env_value
	shr     ax, 1
	shr     ax, 1
	shr     ax, 1
	shr     ax, 1
	shr     ax, 1
	shr     ax, 1
	mov     bl, ah
	mov     ah, al
	mov     al, OPL_FNUM
	add     al, channel
	call    OplWrite
	mov     ax, [si].v_keyon
	or      ah, bl
	mov     al, OPL_KEYON
	add     al, channel
	call    OplWrite
@@step:
	mov     ax, [si].v_pitch.env_step
	add     ax, [si].v_pitch.env_value
	mov     [si].v_pitch.env_value, ax
	dec     word ptr [si].v_pitch.env_count
	jne     @@feedback
	mov     bx, v_pitch
	call    NextStep
@@feedback:
	mov     ax, [si].v_feedback.env_step
	add     ax, [si].v_feedback.env_value
	mov     [si].v_feedback.env_value, ax
	dec     word ptr [si].v_feedback.env_count
	jne     @@mult1
	mov     bx, v_feedback
	call    NextStep
@@mult1:
	mov     ax, [si].v_mult1.env_step
	add     ax, [si].v_mult1.env_value
	mov     [si].v_mult1.env_value, ax
	dec     word ptr [si].v_mult1.env_count
	jne     @@mult2
	mov     bx, v_mult1
	call    NextStep
@@mult2:
	mov     ax, [si].v_mult2.env_step
	add     ax, [si].v_mult2.env_value
	mov     [si].v_mult2.env_value, ax
	dec     word ptr [si].v_mult2.env_count
	jne     @@priority
	mov     bx, v_mult2
	call    NextStep
@@priority:
	mov     ax, [si].v_priority.env_step
	add     ax, [si].v_priority.env_value
	mov     [si].v_priority.env_value, ax
	dec     word ptr [si].v_priority.env_count
	jne     @@wave
	mov     bx, v_priority
	call    NextStep
@@wave:
	mov     ax, [si].v_wave.env_step
	add     ax, [si].v_wave.env_value
	mov     [si].v_wave.env_value, ax
	dec     word ptr [si].v_wave.env_count
	jne     @@done
	mov     bx, v_wave
	jmp     NextStep
@@done:
	ret
UpdateVoice    ENDP

; Read the first step of all eight envelopes.
StartEnvelopes PROC NEAR
	mov     bx, v_pitch
	call    NextStep
	mov     bx, v_level1
	call    NextStep
	mov     bx, v_level2
	call    NextStep
	mov     bx, v_feedback
	call    NextStep
	mov     bx, v_mult1
	call    NextStep
	mov     bx, v_mult2
	call    NextStep
	mov     bx, v_priority
	call    NextStep
	mov     bx, v_wave
	jmp     NextStep
StartEnvelopes ENDP

; Step envelope BX of the voice at SI, reading on when its step runs out.
StepEnvelope   PROC NEAR
	mov     ax, [bx+si].env_step
	add     ax, [bx+si].env_value
	mov     [bx+si].env_value, ax
	dec     word ptr [bx+si].env_count
	je      NextStep
	ret
StepEnvelope   ENDP

; Read envelope BX's script up to its next step. A pair with no ticks jumps
; by its value, -1 ticks sets the envelope's value and -2 sets register bits.
; Ten such commands in a row end the script.
NextStep   PROC NEAR
	mov     di, [bx+si].env_script
	mov     cx, 10
@@read:
	mov     ax, es:[di]
	inc     di
	inc     di
	mov     [bx+si].env_count, ax
	mov     dx, ax
	mov     ax, es:[di]
	inc     di
	inc     di
	mov     [bx+si].env_step, ax
	mov     [bx+si].env_script, di
	cmp     dx, 0
	jne     @@value
	add     di, ax
	sub     di, 4
@@next:
	loop    @@read
; the script went nowhere: hold still for good
	xor     ax, ax
	mov     [bx+si].env_step, ax
	mov     ax, 0FFFFh
	mov     [bx+si].env_count, ax
	ret
@@value:
	cmp     dx, -1
	jne     @@bits
	mov     [bx+si].env_value, ax
	jmp     @@next
@@bits:
	cmp     dx, -2
	jne     @@done
	cmp     bx, v_mult1
	jne     @@mult2Bits
	mov     dx, [si].v_mult_flags
	mov     dh, al
	mov     [si].v_mult_flags, dx
	jmp     @@next
@@mult2Bits:
	cmp     bx, v_mult2
	jne     @@level1Bits
	mov     dx, [si].v_mult_flags
	mov     dl, al
	mov     [si].v_mult_flags, dx
	jmp     @@next
@@level1Bits:
	cmp     bx, v_level1
	jne     @@level2Bits
	mov     dx, [si].v_ksl
	mov     dh, al
	mov     [si].v_ksl, dx
	jmp     @@next
@@level2Bits:
	cmp     bx, v_level2
	jne     @@keyonBits
	mov     dx, [si].v_ksl
	mov     dl, al
	mov     [si].v_ksl, dx
	jmp     @@next
@@keyonBits:
	cmp     bx, v_pitch
	jne     @@connectBit
	mov     [si].v_keyon, ax
	jmp     @@next
@@connectBit:
	cmp     bx, v_feedback
	jne     @@done
	mov     [si].v_connect, ax
	jmp     @@next
@@done:
	ret
NextStep   ENDP

; Step a released voice's level envelope BX. Once the level is below 400h,
; or would pass zero, it stays at zero and counts in `ended'.
FadeEnvelope   PROC NEAR
	mov     ax, [bx+si].env_value
	or      ax, ax
	je      @@ended
	js      @@loud
	cmp     ax, 1024
	jl      @@silence
@@loud:
	mov     ax, [bx+si].env_step
	or      ax, ax
	jns     @@rising
	add     ax, [bx+si].env_value
	jb      @@store
@@silence:
	xor     ax, ax
	mov     [bx+si].env_value, ax
	mov     [bx+si].env_step, ax
	mov     ax, 0FFFFh
	mov     [bx+si].env_count, ax
@@ended:
	inc     word ptr ended
	ret
@@rising:
	add     ax, [bx+si].env_value
@@store:
	mov     [bx+si].env_value, ax
	dec     word ptr [bx+si].env_count
	jne     @@done
	jmp     NextStep
@@done:
	ret
FadeEnvelope   ENDP

; Give the voice at SI the volumeTable row for volume DX, 0 to 127.
SetVolume  PROC NEAR
	and     dx, 78h
	shl     dx, 1
	shl     dx, 1
	shl     dx, 1
	add     dx, offset volumeTable
	mov     [si].v_volume, dx
	ret
SetVolume  ENDP

; Start the sound in segment `sound' at `volume'. Returns its voice, or 0
; when all of them are busy.
_StartAdlibSound    PROC FAR
	ARG     sound:WORD, volume:WORD
	push    bp
	mov     bp, sp
	push    di
	push    si
	push    es
	push    ds
	push    cs
	pop     ds
	mov     ax, sound
	mov     dx, volume
	call    StartSound
	pop     ds
	pop     es
	pop     si
	pop     di
	mov     sp, bp
	pop     bp
	ret
_StartAdlibSound    ENDP

; Start the sound at segment AX, volume DX, on a free voice, and return the
; voice in AX, or 0. The voice also takes a free channel if there is one.
StartSound PROC NEAR
	mov     es, ax
	mov     si, offset voices
	mov     cx, NVOICES
@@findVoice:
	mov     ax, [si].v_state
	or      ax, ax
	je      @@found
	add     si, SIZE VOICEREC
	loop    @@findVoice
	xor     si, si
	jmp     @@done
@@found:
	mov     cx, NCHANNELS
	xor     di, di
@@findChannel:
	mov     al, channelBusy[di]
	or      al, al
	je      @@claim
	inc     di
	loop    @@findChannel
	jmp     @@setChannel                ; none free: DI is NCHANNELS
@@claim:
	inc     al
	mov     channelBusy[di], al
@@setChannel:
	mov     [si].v_channel, di
	call    SetVolume
	call    AttackVoice
@@done:
	mov     ax, si
	ret
StartSound ENDP

; Start the voice at SI on the sound at ES: every envelope on its attack
; script, then its channel set up for the held note.
AttackVoice    PROC NEAR
	mov     ax, es
	mov     [si].v_sound, ax
	mov     ax, 2020h                   ; sustain on for both operators
	mov     [si].v_mult_flags, ax
	mov     ah, 28h                     ; key on, block 2
	mov     [si].v_keyon, ax
	xor     ax, ax
	mov     [si].v_ksl, ax
	mov     [si].v_connect, ax
	mov     ax, es:[snd_hold]
	mov     [si].v_hold, ax
	mov     ax, es:[snd_pitch+part_start]
	mov     [si].v_pitch.env_value, ax
	mov     ax, es:[snd_pitch+part_attack]
	mov     [si].v_pitch.env_script, ax
	mov     ax, es:[snd_level1+part_start]
	mov     [si].v_level1.env_value, ax
	mov     ax, es:[snd_level1+part_attack]
	mov     [si].v_level1.env_script, ax
	mov     ax, es:[snd_level2+part_start]
	mov     [si].v_level2.env_value, ax
	mov     ax, es:[snd_level2+part_attack]
	mov     [si].v_level2.env_script, ax
	mov     ax, es:[snd_feedback+part_start]
	mov     [si].v_feedback.env_value, ax
	mov     ax, es:[snd_feedback+part_attack]
	mov     [si].v_feedback.env_script, ax
	mov     ax, es:[snd_mult1+part_start]
	mov     [si].v_mult1.env_value, ax
	mov     ax, es:[snd_mult1+part_attack]
	mov     [si].v_mult1.env_script, ax
	mov     ax, es:[snd_mult2+part_start]
	mov     [si].v_mult2.env_value, ax
	mov     ax, es:[snd_mult2+part_attack]
	mov     [si].v_mult2.env_script, ax
	mov     ax, es:[snd_priority+part_start]
	mov     [si].v_priority.env_value, ax
	mov     ax, es:[snd_priority+part_attack]
	mov     [si].v_priority.env_script, ax
	mov     ax, es:[snd_wave+part_start]
	mov     [si].v_wave.env_value, ax
	mov     ax, es:[snd_wave+part_attack]
	mov     [si].v_wave.env_script, ax
	call    StartEnvelopes
	mov     ax, HOLDING
	call    ProgramChannel
	mov     ax, HOLDING
	mov     [si].v_state, ax
	ret
AttackVoice    ENDP

; Key off the voice's channel, quieten it, and load the sound's AD and SR
; bytes for state AX. A sound whose scripts start straight after the
; header has none, and gets a slow default.
ProgramChannel PROC NEAR
	push    es
	mov     di, [si].v_channel
	cmp     di, NCHANNELS
	jne     @@load
	jmp     @@done
@@load:
	mov     dx, ax
	mov     ax, [si].v_sound
	mov     es, ax
	mov     bx, 0FF0Fh
	mov     cx, bx
	mov     ax, es:[snd_pitch+part_attack]
	cmp     ax, snd_adsr
	je      @@write
	mov     bx, es:[snd_adsr]
	mov     cx, es:[snd_adsr+2]
	cmp     dx, RELEASING
	jne     @@write
	mov     bx, es:[snd_release_adsr]
	mov     cx, es:[snd_release_adsr+2]
@@write:
	mov     al, opOffsets[di]
	mov     op1, al
	add     al, 3
	mov     op2, al
	xchg    cl, ch
	xchg    cl, bl
	push    cx
	push    bx
	mov     bx, [si].v_channel
	mov     channel, bl
	xor     ah, ah
	mov     al, OPL_KEYON
	add     al, channel
	call    OplWrite
	mov     ah, 3Fh
	mov     al, OPL_LEVEL
	add     al, op1
	call    OplWrite
	mov     al, OPL_LEVEL
	add     al, op2
	call    OplWrite
	mov     ah, 0FFh
	mov     al, OPL_AD
	add     al, op1
	call    OplWrite
	mov     al, OPL_AD
	add     al, op2
	call    OplWrite
	mov     ah, 0FFh
	mov     al, OPL_SR
	add     al, op1
	call    OplWrite
	mov     al, OPL_SR
	add     al, op2
	call    OplWrite
	pop     bx
	pop     cx
	push    cx
	mov     ah, bh
	mov     al, OPL_AD
	add     al, op1
	call    OplWrite
	mov     ah, bl
	mov     al, OPL_AD
	add     al, op2
	call    OplWrite
	pop     bx
	mov     ah, bl
	mov     al, OPL_SR
	add     al, op1
	call    OplWrite
	mov     ah, bh
	mov     al, OPL_SR
	add     al, op2
	call    OplWrite
@@done:
	pop     es
	ret
ProgramChannel ENDP

; Release the voice at SI: every envelope moves to its release script and
; the channel takes the sound's released AD and SR bytes.
ReleaseVoice   PROC NEAR
	mov     ax, 2020h
	mov     [si].v_mult_flags, ax
	mov     ah, 28h
	mov     [si].v_keyon, ax
	xor     ax, ax
	mov     [si].v_ksl, ax
	mov     [si].v_connect, ax
	dec     word ptr [si].v_state       ; HOLDING to RELEASING
	mov     ax, [si].v_sound
	mov     es, ax
	mov     ax, es:[snd_pitch+part_release]
	mov     [si].v_pitch.env_script, ax
	mov     ax, es:[snd_level1+part_release]
	mov     [si].v_level1.env_script, ax
	mov     ax, es:[snd_level2+part_release]
	mov     [si].v_level2.env_script, ax
	mov     ax, es:[snd_feedback+part_release]
	mov     [si].v_feedback.env_script, ax
	mov     ax, es:[snd_mult1+part_release]
	mov     [si].v_mult1.env_script, ax
	mov     ax, es:[snd_mult2+part_release]
	mov     [si].v_mult2.env_script, ax
	mov     ax, es:[snd_priority+part_release]
	mov     [si].v_priority.env_script, ax
	mov     ax, es:[snd_wave+part_release]
	mov     [si].v_wave.env_script, ax
	call    StartEnvelopes
	mov     ax, [si].v_channel
	cmp     ax, NCHANNELS
	je      @@done
	mov     bx, 0FF0Fh
	mov     cx, bx
	mov     ax, es:[snd_pitch+part_attack]
	cmp     ax, snd_adsr
	je      @@write
	mov     bx, es:[snd_release_adsr]
	mov     cx, es:[snd_release_adsr+2]
@@write:
	xchg    cl, ch
	xchg    cl, bl
	push    cx
	mov     ah, bh
	mov     al, OPL_AD
	add     al, op1
	call    OplWrite
	mov     ah, bl
	mov     al, OPL_AD
	add     al, op2
	call    OplWrite
	pop     bx
	mov     ah, bl
	mov     al, OPL_SR
	add     al, op1
	call    OplWrite
	mov     ah, bh
	mov     al, OPL_SR
	add     al, op2
	jmp     OplWrite
@@done:
	ret
ReleaseVoice   ENDP

; Free the voice at SI and key its channel off. The first voice left
; waiting for a channel takes it over; otherwise the channel is free.
FreeVoice  PROC NEAR
	mov     word ptr [si].v_state, 0
	mov     al, channel
	cmp     al, NCHANNELS
	je      @@handOver
	xor     ah, ah
	add     al, OPL_KEYON
	call    OplWrite
@@handOver:
	mov     bx, [si].v_channel
	push    si
	mov     si, offset voices
	mov     cx, NVOICES
@@find:
	mov     ax, [si].v_state
	or      ax, ax
	je      @@next
	mov     ax, [si].v_channel
	cmp     ax, NCHANNELS
	jne     @@next
	mov     [si].v_channel, bx
	mov     ax, [si].v_state
	call    ProgramChannel
	pop     si
	ret
@@next:
	add     si, SIZE VOICEREC
	loop    @@find
	pop     si
	xor     bh, bh
	mov     channelBusy[bx], bh
	ret
FreeVoice  ENDP

; With more voices playing than channels, hand the channels to the voices
; of highest priority, taking one from a lower voice where needed.
ShareChannels  PROC NEAR
	mov     di, offset priority
	mov     si, offset voices
	mov     word ptr playing, 0
	mov     cx, NVOICES
@@rank:
	mov     ax, [si].v_state
	or      ax, ax
	je      @@store                     ; a free voice ranks 0
	inc     word ptr playing
	mov     ax, [si].v_priority.env_value
	or      ax, ax
	jne     @@bias
	inc     ax
@@bias:
	sub     ax, 7FFFh
	jne     @@store
	inc     ax
@@store:
	mov     [di], ax
	inc     di
	inc     di
	add     si, SIZE VOICEREC
	loop    @@rank
	mov     ax, playing
	cmp     ax, NCHANNELS
	jg      @@crowded
	jmp     @@done
@@crowded:
	mov     word ptr playing, NCHANNELS
; take the highest voice not yet placed
@@pickHighest:
	mov     di, offset voices
	xor     cx, cx
	xor     dx, dx
	xor     bx, bx
@@scanHigh:
	mov     ax, priority[bx]
	or      ax, ax
	je      @@nextHigh
	cmp     ax, cx
	jle     @@nextHigh
	mov     dx, di
	mov     cx, ax
	mov     si, bx
@@nextHigh:
	inc     bx
	inc     bx
	add     di, SIZE VOICEREC
	cmp     bx, NVOICES * 2
	jne     @@scanHigh
	or      dx, dx
	je      @@done
	mov     cx, priority[si]
	xor     ax, ax
	mov     priority[si], ax
	mov     di, dx
	mov     ax, [di].v_channel
	cmp     ax, NCHANNELS
	jne     @@placed
; it has no channel: take the one of the lowest voice holding one
	push    di
	mov     si, offset voices
	xor     dx, dx
	xor     bx, bx
@@scanLow:
	mov     ax, priority[bx]
	or      ax, ax
	je      @@nextLow
	cmp     ax, cx
	jge     @@nextLow
	mov     ax, [si].v_channel
	cmp     ax, NCHANNELS
	je      @@nextLow
	mov     ax, priority[bx]
	mov     dx, si
	mov     cx, ax
	mov     di, bx
@@nextLow:
	inc     bx
	inc     bx
	add     si, SIZE VOICEREC
	cmp     bx, NVOICES * 2
	jne     @@scanLow
	mov     si, di
	pop     di
	or      dx, dx
	je      @@done
	mov     word ptr priority[si], 0
	mov     si, dx
	mov     ax, [si].v_channel
	mov     [di].v_channel, ax
	push    si
	mov     si, di
	mov     ax, [si].v_state
	call    ProgramChannel
	pop     si
	mov     word ptr [si].v_channel, NCHANNELS
@@placed:
	dec     word ptr playing
	mov     ax, playing
	or      ax, ax
	je      @@done
	jmp     @@pickHighest
@@done:
	ret
ShareChannels  ENDP

; The timer tick. Advance every playing voice, release those whose hold
; has run out, free those that have faded, then share out the channels.
_TickAdlibSounds    PROC FAR
	push    bp
	mov     bp, sp
	push    di
	push    si
	push    es
	push    ds
	push    cs
	pop     ds
	inc     word ptr ticks
	mov     si, offset voices
	mov     cx, NVOICES
@@voice:
	push    cx
	mov     ax, [si].v_state
	or      ax, ax
	je      @@next
	mov     bx, [si].v_channel
	mov     channel, bl
	mov     di, [si].v_sound
	mov     es, di
	mov     bl, opOffsets[bx]
	mov     op1, bl
	add     bl, 3
	mov     op2, bl
	cmp     ax, HOLDING
	je      @@holding
	call    UpdateVoice
	xor     ax, ax
	mov     ended, ax
	mov     bx, v_level1
	call    FadeEnvelope
	mov     bx, v_level2
	call    FadeEnvelope
	mov     ax, ended
	cmp     ax, 2                       ; both levels faded out
	jne     @@next
	call    FreeVoice
	jmp     @@next
@@holding:
	call    UpdateVoice
	mov     bx, v_level1
	call    StepEnvelope
	mov     bx, v_level2
	call    StepEnvelope
	dec     word ptr [si].v_hold
	jne     @@next
	call    ReleaseVoice
@@next:
	pop     cx
	add     si, SIZE VOICEREC
	loop    @@voice
	call    ShareChannels
	pop     ds
	pop     es
	pop     si
	pop     di
	mov     sp, bp
	pop     bp
	ret
_TickAdlibSounds    ENDP

; Reset the chip and every table, quieten each channel, and report each
; channel handed back.
_ResetAdlib PROC FAR
	push    bp
	mov     bp, sp
	push    di
	push    si
	push    es
	push    ds
	push    cs
	pop     ds
	mov     ax, 0FFFFh                  ; so that every register is written below
	push    ds
	pop     es
	mov     di, offset oplRegs
	mov     cx, 128
	rep     stosw
	xor     ax, ax
	mov     di, offset voices
	mov     cx, NVOICES * SIZE VOICEREC / 2
	rep     stosw
	xor     ax, ax
@@clearRegs:
	push    ax
	call    OplWrite
	pop     ax
	inc     al
	cmp     al, 0
	jne     @@clearRegs
	mov     cx, NCHANNELS
	mov     di, offset channelBusy
@@freeChannels:
	mov     [di], ah
	inc     di
	loop    @@freeChannels
	mov     ah, 20h
	mov     al, OPL_TEST
	call    OplWrite
	xor     ah, ah
	mov     al, OPL_RHYTHM
	call    OplWrite
	xor     ah, ah
	mov     al, OPL_CSM
	call    OplWrite
	mov     cx, NCHANNELS
	xor     si, si
@@quieten:
	push    cx
	mov     bl, opOffsets[si]
	mov     op1, bl
	add     bl, 3
	mov     op2, bl
	mov     ah, 3Fh
	mov     al, OPL_LEVEL
	add     al, op1
	call    OplWrite
	mov     al, OPL_LEVEL
	add     al, op2
	call    OplWrite
	mov     ah, 0FFh
	mov     al, OPL_AD
	add     al, op1
	call    OplWrite
	mov     al, OPL_AD
	add     al, op2
	call    OplWrite
	mov     ah, 0Fh
	mov     al, OPL_SR
	add     al, op1
	call    OplWrite
	mov     al, OPL_SR
	add     al, op2
	call    OplWrite
	pop     cx
	inc     si
	loop    @@quieten
	xor     ax, ax
@@report:
	push    ax
	push    ax
	call    dword ptr ss:_DriverReleaseChannelEntry
	pop     ax
	inc     ax
	cmp     ax, NCHANNELS
	jne     @@report
	pop     ds
	pop     es
	pop     si
	pop     di
	mov     sp, bp
	pop     bp
	ret
_ResetAdlib ENDP

; Write AH to OPL register AL unless the register already holds it.
OplWrite   PROC NEAR
	pushf
	cli
	push    bx
	mov     bl, al
	xor     bh, bh
	add     bx, offset oplRegs
	cmp     ah, [bx]
	je      @@same
	mov     [bx], ah
	push    si
; the chip wants a pause after each port write; reading it back makes one
	mov     dx, ADLIB_PORT
	out     dx, al
	in      al, dx
	in      al, dx
	in      al, dx
	in      al, dx
	in      al, dx
	in      al, dx
	mov     dx, ADLIB_PORT
	inc     dx
	mov     al, ah
	out     dx, al
	in      al, dx
	in      al, dx
	in      al, dx
	in      al, dx
	in      al, dx
	in      al, dx
	in      al, dx
	in      al, dx
	in      al, dx
	in      al, dx
	in      al, dx
	in      al, dx
	in      al, dx
	in      al, dx
	in      al, dx
	in      al, dx
	in      al, dx
	in      al, dx
	in      al, dx
	in      al, dx
	in      al, dx
	in      al, dx
	in      al, dx
	in      al, dx
	in      al, dx
	in      al, dx
	in      al, dx
	in      al, dx
	in      al, dx
	in      al, dx
	in      al, dx
	in      al, dx
	in      al, dx
	in      al, dx
	in      al, dx
	in      al, dx
	in      al, dx
	in      al, dx
	in      al, dx
	in      al, dx
	in      al, dx
	in      al, dx
	pop     si
@@same:
	pop     bx
	popf
	ret
OplWrite   ENDP

; Unused: wait until CX timer counts have passed.
WaitTimer  PROC NEAR
	xor     si, si
	mov     al, 6
	out     43h, al
	mov     al, 36h
	in      al, 40h
	mov     bl, al
	in      al, 40h
	mov     bh, al
@@poll:
	mov     dx, bx
	mov     al, 6
	out     43h, al
	mov     al, 36h
	in      al, 40h
	mov     bl, al
	in      al, 40h
	mov     bh, al
	sub     dx, bx
	jae     @@add
	add     dx, 19886                   ; the counter wrapped
@@add:
	add     si, dx
	cmp     si, cx
	jb      @@poll
	ret
WaitTimer  ENDP

; Stop `voice' at once.
_StopAdlibVoice PROC FAR
	ARG     voice:WORD
	push    bp
	mov     bp, sp
	push    di
	push    si
	push    es
	push    ds
	push    cs
	pop     ds
	mov     si, voice
	call    StopVoice
	pop     ds
	pop     es
	pop     si
	pop     di
	mov     sp, bp
	pop     bp
	ret
_StopAdlibVoice ENDP

; Set the volume of `voice'.
_SetAdlibVolume PROC FAR
	ARG     voice:WORD, volume:WORD
	push    bp
	mov     bp, sp
	push    di
	push    si
	push    es
	push    ds
	push    cs
	pop     ds
	mov     si, voice
	mov     dx, volume
	call    SetVolume
	pop     ds
	pop     es
	pop     si
	pop     di
	mov     sp, bp
	pop     bp
	ret
_SetAdlibVolume ENDP

; The state of `voice'; 0 once it has ended.
_GetAdlibVoiceState PROC FAR
	ARG     voice:WORD
	push    bp
	mov     bp, sp
	push    di
	push    si
	push    es
	push    ds
	push    cs
	pop     ds
	mov     si, voice
	mov     ax, [si].v_state
	pop     ds
	pop     es
	pop     si
	pop     di
	mov     sp, bp
	pop     bp
	ret
_GetAdlibVoiceState ENDP

; Free the voice at SI now, unless it already is.
StopVoice  PROC NEAR
	mov     ax, [si].v_state
	or      ax, ax
	jne     @@stop
	ret
@@stop:
	mov     di, [si].v_sound
	mov     es, di
	mov     bx, [si].v_channel
	mov     channel, bl
	mov     bl, opOffsets[bx]
	mov     op1, bl
	add     bl, 3
	mov     op2, bl
	jmp     FreeVoice
StopVoice  ENDP

; Step segment AX past CX sounds of a bank.
FindBankSound PROC NEAR
	jcxz    @@done
@@skip:
	mov     es, ax
	mov     bx, es:[snd_size]
	shr     bx, 1
	shr     bx, 1
	shr     bx, 1
	shr     bx, 1
	add     ax, bx
	loop    @@skip
@@done:
	ret
FindBankSound ENDP

; Start sound `number' of the bank at segment `bank', at `volume'.
_StartAdlibBankSound    PROC FAR
	ARG     bank:WORD, number:WORD, volume:WORD
	push    bp
	mov     bp, sp
	push    di
	push    si
	push    es
	push    ds
	push    cs
	pop     ds
	mov     ax, bank
	mov     cx, number
	mov     dx, volume
	call    FindBankSound
	call    StartSound
	pop     ds
	pop     es
	pop     si
	pop     di
	mov     sp, bp
	pop     bp
	ret
_StartAdlibBankSound    ENDP

; Lend out channels 2 to 8: report each one, mark it taken, and take it
; from any voice using it.
_YieldAdlibChannels PROC FAR
	push    bp
	mov     bp, sp
	push    di
	push    si
	push    es
	push    ds
	push    cs
	pop     ds
	pushf
	cli
	mov     ax, suspended
	or      ax, ax
	jne     @@done
	mov     bx, 2
@@channel:
	push    bx
	push    bx
	call    dword ptr ss:_DriverClaimChannelEntry
	pop     bx
	mov     al, channelBusy[bx]
	mov     channelBusy[bx], 1
	or      al, al
	je      @@nextChannel
	mov     si, offset voices
	mov     cx, NVOICES
@@voice:
	mov     ax, [si].v_state
	or      ax, ax
	je      @@nextVoice
	mov     dx, [si].v_channel
	cmp     dx, bx
	jne     @@nextVoice
	mov     word ptr [si].v_channel, NCHANNELS
	mov     al, dl
	xor     ah, ah
	add     al, OPL_KEYON
	call    OplWrite
@@nextVoice:
	add     si, SIZE VOICEREC
	loop    @@voice
@@nextChannel:
	inc     bx
	cmp     bx, NCHANNELS
	jne     @@channel
	mov     word ptr suspended, 1
@@done:
	popf
	pop     ds
	pop     es
	pop     si
	pop     di
	mov     sp, bp
	pop     bp
	ret
_YieldAdlibChannels ENDP

; Write AH to OPL register AL whatever it held.
OplWriteForced    PROC NEAR
	mov     bl, al
	xor     bh, bh
	add     bx, offset oplRegs
	mov     dl, al
	xor     dl, 0FFh
	mov     [bx], dl
	jmp     OplWrite
OplWriteForced    ENDP

; Take back channels 2 to 8: report each one, reset its registers and
; mark it free.
_ReclaimAdlibChannels   PROC FAR
	push    bp
	mov     bp, sp
	push    di
	push    si
	push    es
	push    ds
	push    cs
	pop     ds
	pushf
	cli
	mov     ax, suspended
	or      ax, ax
	jne     @@reclaim
	jmp     @@done
@@reclaim:
	mov     bx, 2
@@channel:
	push    bx
	push    bx
	call    dword ptr ss:_DriverReleaseChannelEntry
	pop     bx
	push    bx
	mov     channel, bl
	mov     bl, opOffsets[bx]
	mov     op1, bl
	add     bl, 3
	mov     op2, bl
	mov     al, OPL_FNUM
	add     al, channel
	xor     ah, ah
	call    OplWriteForced
	mov     al, OPL_KEYON
	add     al, channel
	xor     ah, ah
	call    OplWriteForced
	mov     al, OPL_LEVEL
	add     al, op1
	mov     ah, 3Fh
	call    OplWriteForced
	mov     al, OPL_LEVEL
	add     al, op2
	mov     ah, 3Fh
	call    OplWriteForced
	mov     al, OPL_FEEDBACK
	add     al, channel
	xor     ah, ah
	call    OplWriteForced
	mov     al, OPL_MULT
	add     al, op1
	xor     ah, ah
	call    OplWriteForced
	mov     al, OPL_MULT
	add     al, op2
	xor     ah, ah
	call    OplWriteForced
	mov     al, OPL_WAVE
	add     al, op1
	xor     ah, ah
	call    OplWriteForced
	mov     al, OPL_WAVE
	add     al, op2
	xor     ah, ah
	call    OplWriteForced
	mov     al, OPL_AD
	add     al, op1
	mov     ah, 0FFh
	call    OplWriteForced
	mov     al, OPL_AD
	add     al, op2
	mov     ah, 0FFh
	call    OplWriteForced
	mov     al, OPL_SR
	add     al, op1
	mov     ah, 0FFh
	call    OplWriteForced
	mov     al, OPL_SR
	add     al, op2
	mov     ah, 0FFh
	call    OplWriteForced
	pop     bx
	mov     channelBusy[bx], bh
	inc     bx
	cmp     bx, NCHANNELS
	je      @@reclaimed
	jmp     @@channel
@@reclaimed:
	mov     word ptr suspended, 0
@@done:
	popf
	pop     ds
	pop     es
	pop     si
	pop     di
	mov     sp, bp
	pop     bp
	ret
_ReclaimAdlibChannels   ENDP

voices      VOICEREC NVOICES dup (<>)
ended       dw  0                                   ; the voice's level envelopes that have faded out
oplRegs     db  256 dup (0)                         ; the last value written to each register
channel     db  0                                   ; the channel being programmed
op1         db  0                                   ; and its two operators
op2         db  3
opOffsets   db  0, 1, 2, 6, 7, 8, 0Ch, 0Dh, 0Eh     ; each channel's first operator; the second is 3 on
channelBusy db  NCHANNELS dup (0)                   ; nonzero while a channel is taken
priority    dw  NVOICES dup (0)                     ; each voice's claim to a channel, worked out every tick
playing     dw  0                                   ; voices playing, then channels left to hand out
ticks       dw  0
suspended   dw  0                                   ; set while channels 2 to 8 are lent out

; sixteen rows of 64 output levels, one per volume step; a level indexes a row
volumeTable db  3Fh, 3Fh, 3Fh, 3Fh, 3Fh, 3Fh, 3Fh, 3Fh, 3Fh, 3Fh, 3Fh, 3Fh, 3Fh, 3Fh, 3Fh, 3Fh
			db  3Eh, 3Eh, 3Eh, 3Eh, 3Eh, 3Eh, 3Eh, 3Eh, 3Eh, 3Eh, 3Eh, 3Eh, 3Eh, 3Eh, 3Eh, 3Eh
			db  3Dh, 3Dh, 3Dh, 3Dh, 3Dh, 3Dh, 3Dh, 3Dh, 3Dh, 3Dh, 3Dh, 3Dh, 3Dh, 3Dh, 3Dh, 3Dh
			db  3Ch, 3Ch, 3Ch, 3Ch, 3Ch, 3Ch, 3Ch, 3Ch, 3Ch, 3Ch, 3Ch, 3Ch, 3Ch, 3Ch, 3Ch, 3Ch
			db  3Fh, 3Fh, 3Fh, 3Fh, 3Fh, 3Fh, 3Fh, 3Fh, 3Eh, 3Eh, 3Eh, 3Eh, 3Eh, 3Eh, 3Eh, 3Eh
			db  3Dh, 3Dh, 3Dh, 3Dh, 3Dh, 3Dh, 3Dh, 3Dh, 3Ch, 3Ch, 3Ch, 3Ch, 3Ch, 3Ch, 3Ch, 3Ch
			db  3Bh, 3Bh, 3Bh, 3Bh, 3Bh, 3Bh, 3Bh, 3Bh, 3Ah, 3Ah, 3Ah, 3Ah, 3Ah, 3Ah, 3Ah, 3Ah
			db  39h, 39h, 39h, 39h, 39h, 39h, 39h, 39h, 38h, 38h, 38h, 38h, 38h, 38h, 38h, 38h
			db  3Fh, 3Fh, 3Fh, 3Fh, 3Fh, 3Fh, 3Eh, 3Eh, 3Eh, 3Eh, 3Eh, 3Dh, 3Dh, 3Dh, 3Dh, 3Dh
			db  3Ch, 3Ch, 3Ch, 3Ch, 3Ch, 3Ch, 3Bh, 3Bh, 3Bh, 3Bh, 3Bh, 3Ah, 3Ah, 3Ah, 3Ah, 3Ah
			db  39h, 39h, 39h, 39h, 39h, 39h, 38h, 38h, 38h, 38h, 38h, 37h, 37h, 37h, 37h, 37h
			db  36h, 36h, 36h, 36h, 36h, 36h, 35h, 35h, 35h, 35h, 35h, 34h, 34h, 34h, 34h, 34h
			db  3Fh, 3Fh, 3Fh, 3Fh, 3Eh, 3Eh, 3Eh, 3Eh, 3Dh, 3Dh, 3Dh, 3Dh, 3Ch, 3Ch, 3Ch, 3Ch
			db  3Bh, 3Bh, 3Bh, 3Bh, 3Ah, 3Ah, 3Ah, 3Ah, 39h, 39h, 39h, 39h, 38h, 38h, 38h, 38h
			db  37h, 37h, 37h, 37h, 36h, 36h, 36h, 36h, 35h, 35h, 35h, 35h, 34h, 34h, 34h, 34h
			db  33h, 33h, 33h, 33h, 32h, 32h, 32h, 32h, 31h, 31h, 31h, 31h, 30h, 30h, 30h, 30h
			db  3Fh, 3Fh, 3Fh, 3Fh, 3Eh, 3Eh, 3Eh, 3Dh, 3Dh, 3Dh, 3Ch, 3Ch, 3Ch, 3Bh, 3Bh, 3Bh
			db  3Ah, 3Ah, 3Ah, 3Ah, 39h, 39h, 39h, 38h, 38h, 38h, 37h, 37h, 37h, 36h, 36h, 36h
			db  35h, 35h, 35h, 35h, 34h, 34h, 34h, 33h, 33h, 33h, 32h, 32h, 32h, 31h, 31h, 31h
			db  30h, 30h, 30h, 30h, 2Fh, 2Fh, 2Fh, 2Eh, 2Eh, 2Eh, 2Dh, 2Dh, 2Dh, 2Ch, 2Ch, 2Ch
			db  3Fh, 3Fh, 3Fh, 3Eh, 3Eh, 3Eh, 3Dh, 3Dh, 3Ch, 3Ch, 3Ch, 3Bh, 3Bh, 3Bh, 3Ah, 3Ah
			db  39h, 39h, 39h, 38h, 38h, 38h, 37h, 37h, 36h, 36h, 36h, 35h, 35h, 35h, 34h, 34h
			db  33h, 33h, 33h, 32h, 32h, 32h, 31h, 31h, 30h, 30h, 30h, 2Fh, 2Fh, 2Fh, 2Eh, 2Eh
			db  2Dh, 2Dh, 2Dh, 2Ch, 2Ch, 2Ch, 2Bh, 2Bh, 2Ah, 2Ah, 2Ah, 29h, 29h, 29h, 28h, 28h
			db  3Fh, 3Fh, 3Fh, 3Eh, 3Eh, 3Dh, 3Dh, 3Ch, 3Ch, 3Ch, 3Bh, 3Bh, 3Ah, 3Ah, 39h, 39h
			db  38h, 38h, 38h, 37h, 37h, 36h, 36h, 35h, 35h, 35h, 34h, 34h, 33h, 33h, 32h, 32h
			db  31h, 31h, 31h, 30h, 30h, 2Fh, 2Fh, 2Eh, 2Eh, 2Eh, 2Dh, 2Dh, 2Ch, 2Ch, 2Bh, 2Bh
			db  2Ah, 2Ah, 2Ah, 29h, 29h, 28h, 28h, 27h, 27h, 27h, 26h, 26h, 25h, 25h, 24h, 24h
			db  3Fh, 3Fh, 3Eh, 3Eh, 3Dh, 3Dh, 3Ch, 3Ch, 3Bh, 3Bh, 3Ah, 3Ah, 39h, 39h, 38h, 38h
			db  37h, 37h, 36h, 36h, 35h, 35h, 34h, 34h, 33h, 33h, 32h, 32h, 31h, 31h, 30h, 30h
			db  2Fh, 2Fh, 2Eh, 2Eh, 2Dh, 2Dh, 2Ch, 2Ch, 2Bh, 2Bh, 2Ah, 2Ah, 29h, 29h, 28h, 28h
			db  27h, 27h, 26h, 26h, 25h, 25h, 24h, 24h, 23h, 23h, 22h, 22h, 21h, 21h, 20h, 20h
			db  3Fh, 3Fh, 3Eh, 3Eh, 3Dh, 3Dh, 3Ch, 3Ch, 3Bh, 3Ah, 3Ah, 39h, 39h, 38h, 38h, 37h
			db  36h, 36h, 35h, 35h, 34h, 34h, 33h, 33h, 32h, 31h, 31h, 30h, 30h, 2Fh, 2Fh, 2Eh
			db  2Dh, 2Dh, 2Ch, 2Ch, 2Bh, 2Bh, 2Ah, 2Ah, 29h, 28h, 28h, 27h, 27h, 26h, 26h, 25h
			db  24h, 24h, 23h, 23h, 22h, 22h, 21h, 21h, 20h, 1Fh, 1Fh, 1Eh, 1Eh, 1Dh, 1Dh, 1Ch
			db  3Fh, 3Fh, 3Eh, 3Eh, 3Dh, 3Ch, 3Ch, 3Bh, 3Ah, 3Ah, 39h, 39h, 38h, 37h, 37h, 36h
			db  35h, 35h, 34h, 34h, 33h, 32h, 32h, 31h, 30h, 30h, 2Fh, 2Fh, 2Eh, 2Dh, 2Dh, 2Ch
			db  2Bh, 2Bh, 2Ah, 2Ah, 29h, 28h, 28h, 27h, 26h, 26h, 25h, 25h, 24h, 23h, 23h, 22h
			db  21h, 21h, 20h, 20h, 1Fh, 1Eh, 1Eh, 1Dh, 1Ch, 1Ch, 1Bh, 1Bh, 1Ah, 19h, 19h, 18h
			db  3Fh, 3Fh, 3Eh, 3Dh, 3Dh, 3Ch, 3Bh, 3Bh, 3Ah, 39h, 39h, 38h, 37h, 37h, 36h, 35h
			db  34h, 34h, 33h, 32h, 32h, 31h, 30h, 30h, 2Fh, 2Eh, 2Eh, 2Dh, 2Ch, 2Ch, 2Bh, 2Ah
			db  29h, 29h, 28h, 27h, 27h, 26h, 25h, 25h, 24h, 23h, 23h, 22h, 21h, 21h, 20h, 1Fh
			db  1Eh, 1Eh, 1Dh, 1Ch, 1Ch, 1Bh, 1Ah, 1Ah, 19h, 18h, 18h, 17h, 16h, 16h, 15h, 14h
			db  3Fh, 3Fh, 3Eh, 3Dh, 3Ch, 3Ch, 3Bh, 3Ah, 39h, 39h, 38h, 37h, 36h, 36h, 35h, 34h
			db  33h, 33h, 32h, 31h, 30h, 30h, 2Fh, 2Eh, 2Dh, 2Dh, 2Ch, 2Bh, 2Ah, 2Ah, 29h, 28h
			db  27h, 27h, 26h, 25h, 24h, 24h, 23h, 22h, 21h, 21h, 20h, 1Fh, 1Eh, 1Eh, 1Dh, 1Ch
			db  1Bh, 1Bh, 1Ah, 19h, 18h, 18h, 17h, 16h, 15h, 15h, 14h, 13h, 12h, 12h, 11h, 10h
			db  3Fh, 3Fh, 3Eh, 3Dh, 3Ch, 3Bh, 3Bh, 3Ah, 39h, 38h, 37h, 37h, 36h, 35h, 34h, 33h
			db  32h, 32h, 31h, 30h, 2Fh, 2Eh, 2Eh, 2Dh, 2Ch, 2Bh, 2Ah, 2Ah, 29h, 28h, 27h, 26h
			db  25h, 25h, 24h, 23h, 22h, 21h, 21h, 20h, 1Fh, 1Eh, 1Dh, 1Dh, 1Ch, 1Bh, 1Ah, 19h
			db  18h, 18h, 17h, 16h, 15h, 14h, 14h, 13h, 12h, 11h, 10h, 10h, 0Fh, 0Eh, 0Dh, 0Ch
			db  3Fh, 3Fh, 3Eh, 3Dh, 3Ch, 3Bh, 3Ah, 39h, 38h, 38h, 37h, 36h, 35h, 34h, 33h, 32h
			db  31h, 31h, 30h, 2Fh, 2Eh, 2Dh, 2Ch, 2Bh, 2Ah, 2Ah, 29h, 28h, 27h, 26h, 25h, 24h
			db  23h, 23h, 22h, 21h, 20h, 1Fh, 1Eh, 1Dh, 1Ch, 1Ch, 1Bh, 1Ah, 19h, 18h, 17h, 16h
			db  15h, 15h, 14h, 13h, 12h, 11h, 10h, 0Fh, 0Eh, 0Eh, 0Dh, 0Ch, 0Bh, 0Ah, 9, 8
			db  3Fh, 3Fh, 3Eh, 3Dh, 3Ch, 3Bh, 3Ah, 39h, 38h, 37h, 36h, 35h, 34h, 33h, 32h, 31h
			db  30h, 30h, 2Fh, 2Eh, 2Dh, 2Ch, 2Bh, 2Ah, 29h, 28h, 27h, 26h, 25h, 24h, 23h, 22h
			db  21h, 21h, 20h, 1Fh, 1Eh, 1Dh, 1Ch, 1Bh, 1Ah, 19h, 18h, 17h, 16h, 15h, 14h, 13h
			db  12h, 12h, 11h, 10h, 0Fh, 0Eh, 0Dh, 0Ch, 0Bh, 0Ah, 9, 8, 7, 6, 5, 4
			db  3Fh, 3Eh, 3Dh, 3Ch, 3Bh, 3Ah, 39h, 38h, 37h, 36h, 35h, 34h, 33h, 32h, 31h, 30h
			db  2Fh, 2Eh, 2Dh, 2Ch, 2Bh, 2Ah, 29h, 28h, 27h, 26h, 25h, 24h, 23h, 22h, 21h, 20h
			db  1Fh, 1Eh, 1Dh, 1Ch, 1Bh, 1Ah, 19h, 18h, 17h, 16h, 15h, 14h, 13h, 12h, 11h, 10h
			db  0Fh, 0Eh, 0Dh, 0Ch, 0Bh, 0Ah, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0

	END
