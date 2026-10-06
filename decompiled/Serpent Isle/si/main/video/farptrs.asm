; Serpent Isle SI.EXE, resident segment 187 (file offset 0x0404a6, 0 bytes): data but no code.
; Turbo Assembler 2.51 /mx rebuilds its empty code segment as shipped.
; Its data is DS:64D4-64F0, between ovrprof.asm's and crtport.asm's. The far pointers hold DGROUP
; with offset 0, which only a segment fixup gives, so the file was assembly. No code reads them.

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
