; Serpent Isle SI.EXE, resident segment 86 (file offset 0x0337c4, 0 bytes): data but no code.
; Turbo Assembler 2.51 /mx rebuilds its empty code segment as shipped.
; Its data is DS:3F6E-3F70, between random.asm's and crawpal.c's. The empty code segment starts one
; byte after random.asm's end, word aligned, so the file was assembly. No code reads the word.

	.MODEL  MEDIUM

	PUBLIC  UnusedPaletteGlobal1

	.DATA
UnusedPaletteGlobal1 dw 0

	END
