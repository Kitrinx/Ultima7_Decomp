; Serpent Isle INTRO.EXE, one module of resident segment 77 (file offsets 0x015498 to 0x015498, 0 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.
; Its data is DS:4226-4228. Its empty word-aligned code segment is the pad byte before freexmm.c.
; Filename inferred.

	.MODEL  MEDIUM

	PUBLIC  _FlatModeFlags

	.DATA
_FlatModeFlags  dw      0

	END
