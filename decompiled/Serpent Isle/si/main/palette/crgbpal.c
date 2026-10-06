/* Serpent Isle SI.EXE, resident segment 91 (file offsets 0x0349a5 to 0x034b40, 411 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "lowlevel.h"
#include "vooalloc.h"
#include "flex.h"
#include "easyfile.h"
#include "debug.h"
#include "oops.h"
#include "crgbpal.h"

char *PaletteFileName = "PALETTES.FLX";

/* read one palette from PALETTES.FLX into a block, allocating it on first use */
void LoadPalette(long *block, int index)
{
	if (index != RED_RAMP_PALETTE) {
		Flex file;

		if (file.open(BuildPath(StaticPath, PaletteFileName, 0))) {
			if (*block == 0)
				AllocatePaletteBlock(block);
			file.readRecordToVoodoo(index, *block, 0);
			file.close();
		}
	} else
		BuildRedPalette(block);
}

/* allocate a 256-color palette block and clear it */
void AllocatePaletteBlock(long *block)
{
	if ((*block = AllocateVoodooMemory(&VoodooXmsBlock, 768L)) == 0)
		ReportOutOfVoodooMemory();
	FillLinear(*block, 0, 768L, 0x111);
}

/* fill entries 1-255 with a repeating red ramp */
void BuildRedPalette(long *block)
{
	int i;
	char red;
	int offset;

	if (*block == 0)
		AllocatePaletteBlock(block);
	red = 16;
	offset = 3;
	for (i = 1; i < 256; i++) {
		if (red == 57)
			red = 16;
		PokeByte(*block + offset, red);
		PokeByte(*block + offset + 1, 0);
		PokeByte(*block + offset + 2, 0);
		offset += 3;
		red++;
	}
}
