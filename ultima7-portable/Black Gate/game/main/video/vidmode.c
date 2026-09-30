/* Black Gate U7.EXE, resident segment 185 (file offsets 0x0401fc to 0x04030f, 275 bytes).
 * Borland C++ 2.0 -mm -O -G rebuilds it byte for byte.
 * Folder inferred from compiler flags and link order.
 */
#include "u7port.h"
#include "plat.h"
#include "arena.h"
#include "screen.h"
#include "vidmode.h"

#define SLOT_COUNT 256

struct View;

int8_t DisplayMode = -1;
int8_t UnusedModeByte = 3;
char BiosVideoModes[] = { 0x13, 0x0d, 0x04, 0x09, 0x07, 0 };  /* BIOS video mode for each display mode */
int16_t SlotRunLengths[SLOT_COUNT]; /* a run's length at each slot it holds, 0 when free */
extern struct View ScreenView;

/* Switches to a display mode. A graphics mode becomes the current one and frees every slot. */
void SetDisplayMode(int8_t mode)
{
	int8_t biosMode = BiosVideoModes[mode];
	int16_t i;

	if (biosMode > 3 && biosMode != 7) {
		DisplayMode = mode;
		for (i = 0; i < SLOT_COUNT; i++)
			SlotRunLengths[i] = 0;
	}
	ApplyModeColors(mode);
	InitVgaScreen(&ScreenView, 0xff);
}

/* Shows the screen as it is, then waits for the next display refresh, so palette changes made
 * between waits are seen. */
void WaitForRetrace(void)
{
	plat_video_present(ScreenPixels());
	plat_video_wait_retrace();
}

/* Takes the first run of size free slots, marking each with size.
 * Returns the first slot, or -1 when no run is long enough. */
int16_t AllocateSlotRun(int16_t size)
{
	int16_t first = 0;
	int16_t length = 0;
	int16_t i, j;

	for (i = 0; i < SLOT_COUNT; i++) {
		if (SlotRunLengths[i] != 0) {
			length = 0;
			first = i + 1;
		} else
			length++;
		if (length == size) {
			for (j = 0; j < size; j++)
				SlotRunLengths[first + j] = size;
			return first;
		}
	}
	return -1;
}

/* Frees the run of slots that starts at first. */
void FreeSlotRun(int16_t first)
{
	int16_t length = SlotRunLengths[first];
	int16_t i;

	for (i = 0; i < length; i++)
		SlotRunLengths[first + i] = 0;
}
