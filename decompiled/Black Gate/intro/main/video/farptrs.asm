; Black Gate INTRO.EXE, one module of resident segment 86 (file offset 0x01a45e, 0 bytes): data but no code.
; Turbo Assembler 2.51 /mx rebuilds its empty code segment as shipped.
; Its data is DS:1C6E-1C8A, between scalfram.asm's and crtport.asm's: U7's farptrs.asm. Its _BSS,
; DS:2B40-2B60, holds ScreenView and Viewport, 16 bytes apart though a view takes 12. They follow
; memmgr.c's _BSS and precede vidpage.c's, which links last; the owner is inferred from that order.

	.MODEL  MEDIUM

	PUBLIC  UnusedVideoPointer1, UnusedVideoPointer2, UnusedVideoWord1, UnusedVideoPointer3
	PUBLIC  UnusedVideoPointer4, UnusedVideoPointer5, UnusedVideoWord2, UnusedVideoWord3, UnusedVideoWord4
	PUBLIC  C ScreenView, C Viewport

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

	.DATA?
; The screen and the viewport drawing goes to, each a view: video segment, row table and clip box.
ScreenView  dw  8 dup (?)
Viewport    dw  8 dup (?)

	END
