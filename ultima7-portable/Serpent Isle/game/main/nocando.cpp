/* Serpent Isle SI.EXE, resident segment 40 (file offsets 0x01d38a to 0x01d434, 170 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include "u7point.h"
#include "mouse.h"
#include "text.h"
#include "systimer.h"

extern "C" void PlaySfx(uint8_t number, uint16_t volume, int16_t pan);

/* Shows the cursor for why an action failed, plays a sound, waits, then restores the cursor. */
extern "C" void ReportNoCanDo(uint8_t why)
{
	uint8_t oldCursor = CursorBase;
	uint8_t cursor;

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
	PlaySfx(43, 255, 64);
	Timer timer;
	Timer_set(&timer, INT32_C(64));
	Timer_wait(&timer);
	SelectMouseCursor(oldCursor);
}
