/* Black Gate U7.EXE, resident segment 183 (file offsets 0x040121 to 0x04019d, 124 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder inferred from compiler flags and link order.
 */

#include "u7port.h"
#include "dosio.h"
#include "colorreg.h"

/* Registers up to 16 color bytes, the list ended by a null pointer. */
void RegisterColorBytes(char *first, ...)
{
	char *arg = first;
	va_list args;

	va_start(args, first);
	ColorByteCount = 0;
	while (ColorByteCount < MAX_COLOR_BYTES && arg != 0) {
		ColorBytes[ColorByteCount++] = arg;
		arg = va_arg(args, char *);
	}
	va_end(args);
	if (arg != 0)
		ReportError(0x2101);
}

/* The byte for operand n: below 16, one holding n itself; above, a registered one. */
char * GetColorByte(int16_t n)
{
	if (n > ColorByteCount + BASE_COLORS)
		ReportError(0x2100);
	if (n < BASE_COLORS)
		return &IdentityColorBytes[n];
	return ColorBytes[n - BASE_COLORS];
}
