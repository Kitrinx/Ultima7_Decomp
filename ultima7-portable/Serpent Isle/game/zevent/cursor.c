/* Serpent Isle SI.EXE, resident segment 116 (file offset 0x03c021, 0 bytes): data but no code.
 * Borland C++ 2.0 -mm -O -G rebuilds its empty code segment as shipped.
 * Its data is DS:61FC-6202, between msclick.c's and systimer.c's, as link order places segment
 * 116. Black Gate keeps these in preload.c; the name is a guess.
 */

#include "u7port.h"
/* where the mouse cursor is drawn, and whether it is */
int16_t CursorX = 0, CursorY = 0;
uint8_t CursorDrawn = 0, CursorTracking = 0;

void ResetCursorGlobals(void)
{
	CursorX = 0;
	CursorY = 0;
	CursorDrawn = 0;
	CursorTracking = 0;
}
