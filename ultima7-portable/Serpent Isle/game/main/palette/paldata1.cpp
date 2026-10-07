/* Serpent Isle SI.EXE, resident segment 86 (file offset 0x0337c4, 0 bytes): data but no code.
 * Turbo Assembler 2.51 /mx rebuilds its empty code segment as shipped.
 * Its data is DS:3F6E-3F70, between random.asm's and crawpal.c's: one word, 0. No code reads it.
 */

#include "u7port.h"
int16_t UnusedPaletteGlobal1 = 0;

extern "C" void ResetPaldata1Globals(void)
{
	UnusedPaletteGlobal1 = 0;
}
