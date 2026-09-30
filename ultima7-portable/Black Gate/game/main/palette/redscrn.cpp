/* Black Gate U7.EXE, resident segment 109 (file offsets 0x037e6d to 0x0381d7, 874 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder inferred from compiler flags and link order.
 */

#include "u7port.h"
#include "lowlevel.h"
#include "view.h"
#include "flex.h"
#include "crawpal.h"
#include "u7manage.h"
#include "redtimer.h"
#include "worldpal.h"
#include "debug.h"
#include "u7sound.h"
#include "plat.h"
#include "redscrn.h"

extern "C" void PlaySfx(uint8_t number, int16_t volume, int16_t pan, int16_t flags);

int32_t RedScreenCycleRate = 1;
int16_t RedScreenShapeNumber = 19;
char *const EndshapeFileName = "static\\endshape.flx";

RedScreen::RedScreen()
{
	visible = 0;
	shapeBlock = -1;
	HookRedScreenTimer();
}

RedScreen::~RedScreen()
{
	UnhookRedScreenTimer();
	visible = 0;
}

/* On a period PC loading kept the red screen up for seconds; hosts load at once. */
#define RED_SCREEN_MINIMUM_MS 4000
static uint32_t RedScreenShownAt;

void RedScreen::show()
{
	RedScreenShownAt = plat_milliseconds();
	load();
	if (DebugOutputEnabled == 0)
		StartRedScreenCycle();
	visible = 1;
}

void RedScreen::hide()
{
	while (plat_milliseconds() - RedScreenShownAt < RED_SCREEN_MINIMUM_MS)
		plat_yield();
	if (DebugOutputEnabled == 0)
		StopRedScreenCycle();
	stop();
	visible = 0;
}

/* read the picture into a shape segment once, then draw it and start the palette cycling */
void RedScreen::load()
{
	FlexEntry where;
	int32_t size;
	Flex file;

	if (DebugOutputEnabled == 0) {
		ClearScreen(0);
		if (shapeBlock == -1) {
			file.open(EndshapeFileName);
			file.getEntry(RedScreenShapeNumber, &where);
			size = where.size;
			shapeBlock = gShapeManager.allocateBlock(size, 0x7fff, 0);
			file.readEntryToVoodoo(&where, gShapeManager.get(shapeBlock), 0);
			file.close();
		}
		paint();
		RedScreenStepTicks = RedScreenCycleRate;
		GameScreen.hold();
		PlaySfx(77, 255, 64, 0);
	}
}

void RedScreen::stop()
{
	if (DebugOutputEnabled == 0) {
		GameScreen.finishFadeOut();
		ClearScreen(0);
		StopSfx(77);
	}
	GameScreen.state = PAL_RESET;
	GameScreen.restore();
}

void RedScreen::paint()
{
	int8_t zero = 0;
	int32_t shape = gShapeManager.get(shapeBlock);

	PokeByte(shape + 7, zero);
	DrawFrame(&ScreenView, 0, 0, shape, 0, 0x111);
}

/* called from the timer: rotate colors 16 to 93 by one and load them */
void CycleRedScreenPalette(void)
{
	GameScreen.pal->cycleRange(1);
	SetPaletteRange(GameScreen.pal->colors, GameScreen.pal->order, 16, 77);
}

extern "C" void ResetRedscrnGlobals(void)
{
	RedScreenCycleRate = 1;
	RedScreenShapeNumber = 19;
	RedScreenShownAt = 0;
}
