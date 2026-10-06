; Serpent Isle ENDGAME.EXE, one module of resident segment 76 (file offsets 0x0147fc to 0x0147fc, 0 bytes).
; Turbo Assembler 2.51 /mx rebuilds it byte for byte.
; Its data is DS:3E88-3E8A. Its empty word-aligned code segment is the pad byte before freexmm.c.
; Filename inferred.

	.MODEL  MEDIUM

	PUBLIC  _FlatModeFlags

	.DATA
_FlatModeFlags  dw      0

	END
