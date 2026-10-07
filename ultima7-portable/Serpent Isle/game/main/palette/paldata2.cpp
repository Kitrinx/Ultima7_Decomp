/* Serpent Isle SI.EXE, resident segment 95 (file offset 0x03530e, 0 bytes): data but no code.
 * Borland C++ 2.0 -mm -O -G rebuilds its empty code segment as shipped.
 * Its data is DS:421C-421E, after redtimer.asm's. No code reads it, so segment 96 or 97 could own
 * it as well; the linked bytes are the same either way.
 */

#include "u7port.h"
int16_t UnusedPaletteGlobal2 = 0;

extern "C" void ResetPaldata2Globals(void)
{
	UnusedPaletteGlobal2 = 0;
}
