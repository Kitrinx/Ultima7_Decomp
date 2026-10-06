/* Serpent Isle INTRO.EXE, resident segment 19 (file offsets 0x00c983 to 0x00c991, 14 bytes).
 * Borland C++ 2.0 -mm -O -1 -P rebuilds it byte for byte as C++.
 */

#include "display.h"

/* Puts the saved video mode back. */
void SavedMode::shutdown()
{
	SetVideoMode(&mode);
}
