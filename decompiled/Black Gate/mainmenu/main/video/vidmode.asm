; Black Gate MAINMENU.EXE, one module of resident segment 72 (file offset 0x019432, 0 bytes): data but no code.
; Turbo Assembler 2.51 /mx rebuilds its empty code segment as shipped.
; Its data is DS:4982-499E, between xmmblock.asm's and crtport.asm's; U7 has the same data in
; farptrs.asm. Its empty VIDMODE_TEXT fixes where vidmode.c's code goes: vidmode.c links after
; modecolr.c, as its data and relocations show, yet its code sits here. So this file was VIDMODE.ASM.

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
