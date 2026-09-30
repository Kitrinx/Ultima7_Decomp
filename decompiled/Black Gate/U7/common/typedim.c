/* Black Gate U7.EXE, resident segment 94 (file offsets 0x0334e8 to 0x03358a, 162 bytes).
 * Borland C++ 2.0 -mm -O -G rebuilds it byte for byte.
 * Folder inferred from Serpent Isle's link groups.
 */

#include "type.h"

/* a flipped item lies with its x and y swapped */
#define IS_FLIPPED(w) ((char) (((w) & 0x8000) == 0x8000))

unsigned GetFootprintX(unsigned far *typeFrame)
{
	if (IS_FLIPPED(*typeFrame))
		return gItemTypeInfo[*typeFrame & 0x3ff].footprintY;
	else
		return gItemTypeInfo[*typeFrame & 0x3ff].footprintX;
}

unsigned GetFootprintY(unsigned far *typeFrame)
{
	if (IS_FLIPPED(*typeFrame))
		return gItemTypeInfo[*typeFrame & 0x3ff].footprintX;
	else
		return gItemTypeInfo[*typeFrame & 0x3ff].footprintY;
}
