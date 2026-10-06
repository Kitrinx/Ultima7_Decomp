/* Black Gate U7.EXE, resident segment 184 (file offsets 0x04019d to 0x0401fc, 95 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder inferred from compiler flags and link order.
 */

#include "colorreg.h"
#include "vidmode.h"

#define IS_LOADED(n) ((unsigned char) (ModeColorMaps[n].map != 0))

/* Set each color operand's byte to its color in the mode's map; a mode with no map drops the
 * registered bytes first. */
extern "C" void ApplyModeColors(char mode)
{
	int i;

	if (!IS_LOADED(mode))
		RegisterColorBytes(0);
	for (i = 0; i < ColorByteCount + BASE_COLORS; i++)
		*GetColorByte(i) = MapColor(&ModeColorMaps[mode], i);
}
