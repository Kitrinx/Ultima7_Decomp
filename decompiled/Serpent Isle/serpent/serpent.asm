; Ultima VII ULTIMA7.COM (690 bytes). Turbo Assembler 2.51 and TLINK /t rebuild it byte for byte.
; Assembled with /dSERPENT it is Serpent Isle's SERPENT.COM (694 bytes).
;
; Runs the game's programs one after another. Each program's exit code picks the next:
;
;   start -> mainmenu m -> exit code -> intro, menu, game, endgame ... -> exit code -> ...

STACK_SIZE      equ 300

; What to run next; the menu, game, intro and endgame exit with one of these
DO_QUIT         equ 1
DO_MENU_V       equ 2
DO_GAME         equ 3
DO_ENDGAME      equ 4
DO_MENU_N       equ 5               ; last program: thank the player and stop
DO_INTRO        equ 6
DO_MENU_C       equ 7
DO_SURPRISE     equ 8
DO_MENU_L       equ 9
DO_MENU_M       equ 10

; Our PSP
CMD_LEN         equ 80h             ; command tail length
CMD_TEXT        equ 81h
FCB1            equ 5Ch
FCB2            equ 6Ch

LONG_TAIL       equ 124             ; no room left for " p"

; RunExec's frame, as depths below BP: the command tail at the bottom, then
; the caller's DTA and Ctrl-Break state, then the EXEC parameter block
FRAME_SIZE      equ 148
OLD_DTA         equ 20
BREAK_STATE     equ 16
EXEC_BLOCK      equ 14
TAIL_MAX        equ 127

; the caller's DI and ES, pushed on entry
NAME_OFFSET     equ 2
NAME_SEGMENT    equ 4

	.MODEL  TINY

	.CODE
	ORG     100h

Start:
	mov     dx, ds
	call    Launch
	xor     al, al
	mov     ah, 4Ch
	int     21h

Launch:
	jmp     Setup

Run:
	call    SaveMode
	mov     ah, 00h
	mov     al, 13h
	int     10h
	mov     al, DO_MENU_M
Dispatch:
	cmp     al, DO_QUIT
	jne     notQuit
	jmp     Done
notQuit:
	cmp     al, DO_INTRO
	jne     notIntro
	mov     di, offset IntroExe
	mov     si, offset Password
	call    RunProgram
	jnc     Dispatch
	jmp     LoadError
notIntro:
	cmp     al, DO_MENU_V
	jne     notMenuV
	mov     di, offset MenuExe
	mov     si, offset ArgV
	call    RunProgram
	jnc     Dispatch
	jmp     LoadError
notMenuV:
	cmp     al, DO_GAME
	jne     notGame
	mov     di, offset GameExe
	; pass our own command line on, with " p" added
	xor     bx, bx
	mov     bl, ds:[CMD_LEN]
	cmp     bx, LONG_TAIL
	jl      tailFits
	sub     bx, 3
tailFits:
	add     bx, CMD_TEXT
	mov     byte ptr [bx], ' '
	inc     bx
	mov     byte ptr [bx], 'p'
	inc     bx
	mov     byte ptr [bx], 0
	mov     si, CMD_TEXT
	call    RunProgram
	jnc     Dispatch
	jmp     LoadError
notGame:
	cmp     al, DO_ENDGAME
	jne     notEndgame
	mov     di, offset EndgameExe
	mov     si, offset Password
	call    RunProgram
	jnc     Dispatch
	jmp     LoadError
notEndgame:
	cmp     al, DO_MENU_N
	jne     notMenuN
	mov     di, offset MenuExe
	mov     si, offset ArgN
	call    RunProgram
	jnc     Thanks
	jmp     LoadError
notMenuN:
	cmp     al, DO_MENU_C
	jne     notMenuC
	mov     di, offset MenuExe
	mov     si, offset ArgC
	call    RunProgram
	jc      LoadError
	jmp     Dispatch
notMenuC:
	cmp     al, DO_SURPRISE
	jne     notSurprise
	mov     di, offset SurpriseExe
	mov     si, offset ArgU1
	call    RunProgram
	jc      LoadError
	jmp     Dispatch
notSurprise:
	cmp     al, DO_MENU_M
	jne     notMenuM
	mov     di, offset MenuExe
	mov     si, offset ArgM
	call    RunProgram
	jc      LoadError
	jmp     Dispatch
notMenuM:
	cmp     al, DO_MENU_L
	jne     Thanks
	mov     di, offset MenuExe
	mov     si, offset ArgL
	call    RunProgram
	jc      LoadError
	jmp     Dispatch

LoadError:
	call    RestoreMode
	call    GetError
	mov     ah, 09h
	mov     dx, offset LoadErrMsg
	int     21h
	jmp     Done
Thanks:
	call    RestoreMode
	mov     ah, 09h
	mov     dx, offset ThanksMsg
	int     21h
Done:
	ret

; Give memory back to DOS, keeping the program and its stack
Setup:
	mov     ah, 4Ah
	mov     bx, offset ProgramEnd
	add     bx, STACK_SIZE
	mov     cl, 4
	shr     bx, cl
	inc     bx
	int     21h
	pop     bx                      ; our return address
	mov     sp, offset ProgramEnd
	add     sp, STACK_SIZE
	push    bx
	jmp     Run

; Each falls through to the next
RestoreMode:
	push    ax
	mov     ah, 00h
	mov     al, OldMode
	int     10h
	pop     ax
SaveMode:
	push    ax
	mov     ah, 0Fh
	int     10h
	mov     OldMode, al
	pop     ax
SetError:
	mov     cs:ErrorCode, ax
	ret

GetError:
	mov     ax, cs:ErrorCode
	ret

; Run the program named at ES:DI with the arguments at DS:SI and the environment at DX.
; Returns carry set and the DOS error in AX when it cannot be loaded.
RunExec:
	push    bx
	push    cx
	push    dx
	push    ds
	push    si
	push    es
	push    di
	push    bp
	mov     bp, sp
	sub     sp, FRAME_SIZE
	; copy the arguments and turn them into a counted, CR-ended command tail
	push    ss
	pop     es
	mov     di, sp
	inc     di
	push    di
	mov     ax, TAIL_MAX
	mov     cx, ax
	rep movsb
	xchg    ax, cx
	mov     ah, cl
	pop     di
	repne scasb
	inc     cx
	sub     ah, cl
	mov     [bp-FRAME_SIZE], ah
	dec     di
	mov     al, 0Dh
	stosb
	; EXEC parameter block: environment, command tail, our two FCBs
	mov     [bp-EXEC_BLOCK], dx
	mov     [bp-EXEC_BLOCK+2], sp
	mov     [bp-EXEC_BLOCK+4], ss
	mov     word ptr [bp-EXEC_BLOCK+6], FCB1
	mov     [bp-EXEC_BLOCK+8], es
	mov     word ptr [bp-EXEC_BLOCK+10], FCB2
	mov     [bp-EXEC_BLOCK+12], es
	; the child may change these
	mov     ah, 2Fh
	int     21h
	mov     [bp-OLD_DTA], bx
	mov     [bp-OLD_DTA+2], es
	mov     ax, 3300h
	int     21h
	mov     [bp-BREAK_STATE], dl
	; DOS 2 loses SS:SP across EXEC
	push    bp
	mov     SaveSP, sp
	mov     SaveSS, ss
	mov     dx, [bp+NAME_OFFSET]
	mov     ds, [bp+NAME_SEGMENT]
	mov     bx, bp
	sub     bx, EXEC_BLOCK
	push    ss
	pop     es
	mov     ax, 4B00h
	int     21h
	xchg    ax, bx                  ; keep the error and flags
	lahf
	xchg    ax, cx
	mov     dx, cs
	mov     ds, dx
	cli
	mov     sp, SaveSP
	mov     ss, SaveSS
	sti
	pop     bp
	mov     dl, [bp-BREAK_STATE]
	mov     ax, 3301h
	int     21h
	lds     dx, [bp-OLD_DTA]
	mov     ah, 1Ah
	int     21h
	xchg    ax, cx
	sahf
	xchg    ax, bx
	jc      execFailed
execDone:
	mov     sp, bp
	pop     bp
	pop     di
	pop     es
	pop     si
	pop     ds
	pop     dx
	pop     cx
	pop     bx
	ret
execFailed:
	call    SetError
	jmp     execDone

; Run the program at DI with the arguments at SI. Returns its exit code in AL, or carry.
RunProgram:
	push    dx
	mov     dx, 0                   ; the child gets a copy of our environment
	call    RunExec
	jc      runFailed
	mov     ah, 4Dh
	int     21h
runFailed:
	pop     dx
	ret

	.DATA
IntroExe        db 'intro.exe', 0
MenuExe         db 'mainmenu.exe', 0
IFDEF SERPENT
GameExe         db 'SI.exe', 0
ELSE
GameExe         db 'u7.exe', 0
ENDIF
EndgameExe      db 'endgame.exe', 0
SurpriseExe     db 'surprise.exe', 0
ArgV            db 'v', 0
ArgN            db 'n', 0
ArgC            db 'c', 0
ArgL            db 'l', 0
ArgU1           db 'u1', 0
ArgM            db 'm', 0
ArgSpace        db ' ', 0
ArgVR           db 'v r', 0
IFDEF SERPENT
ThanksMsg       db 'Thank you for playing Ultima VII Part II : Serpent Isle!', 13, 13, '$'
ELSE
ThanksMsg       db 'Thank you for playing Ultima VII, The Black Gate!', 13, 13, '$'
ENDIF
LevelMsg        db 'Error level is 00', 13, '$'
LoadErrMsg      db 'An error had occurred loading a program.', 13, 13, '$'
; given to intro and endgame
IFDEF SERPENT
Password        db 'hisss', 0
ELSE
Password        db 'ereiamjh', 0
ENDIF
OldMode         db 0
ErrorCode       dw 0FFFFh
SaveSP          dw ?
SaveSS          dw ?
ProgramEnd      label byte

	END     Start
