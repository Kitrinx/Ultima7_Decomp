/* Black Gate U7.EXE, resident segment 111 (file offset 0x0382ec, 0 bytes): data but no code.
 * Borland C++ 2.0 -mm -O -G rebuilds its empty code segment as shipped.
 * Its data is DS:5F70-5F72, between redtimer.asm's and the first G3 overlay's. No code reads it,
 * so any module from segment 110 to 116 could own it; the linked bytes are the same either way.
 */

#include "u7port.h"
int16_t UnusedPaletteGlobal2 = 0;

extern "C" void ResetPaldata2Globals(void)
{
	UnusedPaletteGlobal2 = 0;
}
