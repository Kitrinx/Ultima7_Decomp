/* Serpent Isle SI.EXE, resident segment 183 (file offsets 0x03fe17 to 0x03fe76, 95 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder chosen by subsystem.
 */

#include "u7port.h"
#include "colormap.h"
#include "vidmode.h"

#define IS_LOADED(n) ((uint8_t) (ModeColorMaps[n].map != 0))

/* Set each color operand's byte to its color in the mode's map; a mode with no map drops the
 * registered bytes first. */
extern "C" void ApplyModeColors(int8_t mode)
{
	int16_t i;

	if (!IS_LOADED(mode))
		RegisterColorBytes(0);
	for (i = 0; i < ColorByteCount + BASE_COLORS; i++)
		*GetColorByte(i) = MapColor(&ModeColorMaps[mode], i);
}
