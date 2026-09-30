/* Black Gate U7.EXE, resident segment 104 (file offsets 0x036835 to 0x036bc7, 914 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder inferred from compiler flags and link order.
 */

#include "lowlevel.h"
#include "dosio.h"
#include "vooalloc.h"
#include "chkfile.h"
#include "gtimer.h"
#include "oops.h"
#include "rgbstep.h"
#include "crawpal.h"

#define CYCLE_RANGE_COUNT 6

/* colors lo to hi rotate by one each cycle */
struct ColorRange {
	int lo;
	int hi;
};

ColorRange PaletteCycleRanges[CYCLE_RANGE_COUNT] = {
	{ 224, 231 }, { 232, 239 }, { 240, 243 }, { 244, 247 }, { 248, 251 }, { 252, 254 }
};

void Palette::apply(unsigned char mode)
{
	int i, unused;

	if (changed() == 0)
		return;
	switch (mode) {
	case 2:
		SetPaletteRange(colors, order, 0, 256);
		break;
	case 0:
		SetPaletteRange(colors, order, 0, 256);
		break;
	case 1:
		SetPaletteRange(colors, order, 0, 256);
		break;
	default:
		/* the loop's body is empty in the shipped code */
		for (i = 0, unused = 0; i < 16; i++, unused += 16)
			;
		break;
	}
	modified = 0;
}

Palette::Palette()
{
	int i, unused;

	modified = 0;
	for (i = 0, unused = 0; i < 256; i++, unused++)
		order[i] = i;
}

Palette::Palette(long src)
{
	int i, unused;

	modified = 0;
	for (i = 0, unused = 0; i < 256; i++, unused++)
		order[i] = i;
	copyColors(src);
}

void Palette::allocate()
{
	if ((colors = AllocateVoodooMemory(&VoodooXmsBlock, PALETTE_SIZE)) == 0)
		ReportOutOfVoodooMemory();
	FillLinear(colors, 0, PALETTE_SIZE, 0x111);
}

void Palette::loadFile(char *name)
{
	RgbColor buf[256];
	DataFile f(name, 1);

	f.read(buf, sizeof buf);
	copyColors(PointerToLinear(buf));
	modified = 1;
	apply(0);
}

void Palette::setColors(long src)
{
	copyColors(src);
	modified = 1;
}

void Palette::cycle()
{
	int i, lo, j, saved;

	if (!GameTime.running())
		return;
	for (i = 0; i < CYCLE_RANGE_COUNT; i++) {
		lo = PaletteCycleRanges[i].lo;
		j = PaletteCycleRanges[i].hi;
		saved = order[j];
		for (; j > lo; j--)
			order[j] = order[j - 1];
		order[j] = saved;
	}
}

/* rotate colors 16 to 93 by one, for the red screen */
void Palette::cycleRange(int)
{
	int i, saved;

	saved = order[93];
	for (i = 93; i > 16; i--)
		order[i] = order[i - 1];
	order[i] = saved;
}

void Palette::copyColors(long src)
{
	int i;

	if (colors == 0)
		allocate();
	PaletteBytesToWords(colors, src);
	for (i = 0; i < 256; i++)
		order[i] = i;
}
