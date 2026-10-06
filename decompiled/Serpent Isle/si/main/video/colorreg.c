/* Serpent Isle SI.EXE, resident segment 182 (file offsets 0x03fd9b to 0x03fe17, 124 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder chosen by subsystem.
 */

#include "dosio.h"
#include "colormap.h"

/* Registers up to 16 color bytes, the list ended by a null pointer. */
void far RegisterColorBytes(char *first, ...)
{
	char **arg = &first;

	ColorByteCount = 0;
	while (ColorByteCount < MAX_COLOR_BYTES && *arg != 0)
		ColorBytes[ColorByteCount++] = *arg++;
	if (*arg != 0)
		ReportError(0x2101);
}

/* The byte for operand n: below 16, one holding n itself; above, a registered one. */
char *far GetColorByte(int n)
{
	if (n > ColorByteCount + BASE_COLORS)
		ReportError(0x2100);
	if (n < BASE_COLORS)
		return &IdentityColorBytes[n];
	return ColorBytes[n - BASE_COLORS];
}
