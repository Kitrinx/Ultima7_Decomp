/* Black Gate U7.EXE, resident segment 128 (file offsets 0x03de7b to 0x03deb9, 62 bytes).
 * Borland C++ 2.0 -mm -O -G rebuilds it byte for byte.
 * Folder inferred from compiler flags and link order.
 */

#include "dosio.h"
#include "colorreg.h"

int ColorByteCount = 0;
char *ColorBytes[MAX_COLOR_BYTES] = { 0 };
char IdentityColorBytes[BASE_COLORS] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
struct ColorMap ModeColorMaps[5];

#define IS_VALID(p) ((unsigned char) ((p) != 0))

unsigned char far MapColor(struct ColorMap *colorMap, int color)
{
	if (color > ColorByteCount + BASE_COLORS)
		ReportError(0x2100);
	if (!IS_VALID(colorMap->map))
		return color;
	return colorMap->map[color];
}
