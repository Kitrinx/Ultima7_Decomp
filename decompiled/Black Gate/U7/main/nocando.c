/* Black Gate U7.EXE, resident segment 64 (file offsets 0x025d2c to 0x025dd9, 173 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Original folder unknown.
 */

#include "u7point.h"
#include "mouse.h"
#include "text.h"
#include "systimer.h"

extern "C" void far PlaySfx(unsigned char number, int volume, int pan, int flags);

/* Shows the cursor for why an action failed, plays a sound, waits, then restores the cursor. */
extern "C" void ReportNoCanDo(unsigned char why)
{
	unsigned char oldCursor = CursorBase;
	unsigned char cursor;

	switch (why) {
	case 3:
		cursor = 5;
		break;
	case 2:
		cursor = 4;
		break;
	case 4:
		cursor = 3;
		break;
	case 5:
		cursor = 6;
		break;
	case 7:
		cursor = 49;
		break;
	default:
		cursor = 1;
		break;
	}
	SelectMouseCursor(cursor);
	PlaySfx(76, 255, 64, 0);
	Timer timer;
	Timer_set(&timer, 64L);
	Timer_wait(&timer);
	SelectMouseCursor(oldCursor);
}
