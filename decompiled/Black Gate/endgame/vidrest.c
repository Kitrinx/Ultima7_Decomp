/* Black Gate ENDGAME.EXE, resident segment 20 (file offsets 0x00c3e3 to 0x00c3f1, 14 bytes).
 * Borland C++ 2.0 -mm -O -1 -P rebuilds it byte for byte as C++.
 */

#include "display.h"

/* Puts the saved video mode back. */
void SavedMode::shutdown()
{
	SetVideoMode(&mode);
}
