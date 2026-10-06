/* Serpent Isle SI.EXE, resident segment 93 (file offsets 0x034eaf to 0x0351f9, 842 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "lowlevel.h"
#include "view.h"
#include "flex.h"
#include "crawpal.h"
#include "u7manage.h"
#include "redtimer.h"
#include "worldpal.h"
#include "debug.h"
#include "redscrn.h"

long RedScreenCycleRate = 1;
int RedScreenShapeNumber = 0;
char *EndshapeFileName = "static\\mainshp.flx";

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

void RedScreen::show()
{
	load();
	if (DebugOutputEnabled == 0)
		StartRedScreenCycle();
	visible = 1;
}

void RedScreen::hide()
{
	if (DebugOutputEnabled == 0)
		StopRedScreenCycle();
	stop();
	visible = 0;
}

/* read the picture into a shape segment once, then draw it and start the palette cycling */
void RedScreen::load()
{
	FlexEntry where;
	long size;
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
	}
}

void RedScreen::stop()
{
	if (DebugOutputEnabled == 0) {
		GameScreen.finishFadeOut();
		ClearScreen(0);
	}
	GameScreen.state = PAL_RESET;
	GameScreen.restore();
}

void RedScreen::paint()
{
	char zero = 0;
	long shape = gShapeManager.get(shapeBlock);

	PokeByte(shape + 7, zero);
	DrawFrame(&ScreenView, 0, 0, shape, 0, 0x111);
}

/* called from the timer: rotate colors 16 to 93 by one and load them */
void far CycleRedScreenPalette(void)
{
	GameScreen.pal->cycleRange(1);
	SetPaletteRange(GameScreen.pal->colors, GameScreen.pal->order, 16, 77);
}
