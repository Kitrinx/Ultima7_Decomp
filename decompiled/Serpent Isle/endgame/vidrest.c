/* Serpent Isle ENDGAME.EXE, resident segment 18 (file offsets 0x00bce7 to 0x00bcf5, 14 bytes).
 * Borland C++ 2.0 -mm -O -1 -P rebuilds it byte for byte as C++.
 */

#include "display.h"

/* Puts the saved video mode back. */
void SavedMode::shutdown()
{
	SetVideoMode(&mode);
}
