; Serpent Isle MAINMENU.EXE, one module of resident segment 71 (file offsets 0x01989e to 0x01989e, 0 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.
; Its data is DS:4882-489E, unused, as in Black Gate MAINMENU's VIDMODE.ASM. Its empty VIDMODE_TEXT
; places vidmode.c's code here although vidmode.c links after modecolr.c.

	.MODEL  MEDIUM

	PUBLIC  UnusedVideoPointer1, UnusedVideoPointer2, UnusedVideoWord1, UnusedVideoPointer3
	PUBLIC  UnusedVideoPointer4, UnusedVideoPointer5, UnusedVideoWord2, UnusedVideoWord3, UnusedVideoWord4

	.DATA
UnusedVideoPointer1 dd  DGROUP:0
UnusedVideoPointer2 dd  DGROUP:0
UnusedVideoWord1    dw  0
UnusedVideoPointer3 dd  DGROUP:0
UnusedVideoPointer4 dd  DGROUP:0
UnusedVideoPointer5 dd  DGROUP:0
UnusedVideoWord2    dw  0
UnusedVideoWord3    dw  0
UnusedVideoWord4    dw  0

	END
