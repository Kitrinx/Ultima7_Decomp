/* Serpent Isle SI.EXE, resident segment 153 (file offsets 0x03f078 to 0x03f153, 219 bytes).
 * Borland C++ 2.0 -mm -O -G rebuilds it byte for byte.
 * Folder chosen by subsystem.
 */

/* path: freexmm.c */
#include "u7port.h"
#include "init.h"
#include "xmmblock.h"
#include "xmminit.h"
#include "freexmm.h"

#define XMM_ERROR(line) FatalError(__FILE__, line)

/* The arena outlives the game, so there is nothing to give back. */
void FreeXMM(void)
{
}

int16_t ShutdownXMM(void)
{
	FreeXMM();
	return 0;
}
