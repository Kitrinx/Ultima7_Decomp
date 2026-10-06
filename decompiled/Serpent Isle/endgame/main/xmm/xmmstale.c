/* Serpent Isle ENDGAME.EXE, resident segment 80 (file offsets 0x014a79 to 0x014ab8, 63 bytes).
 * Borland C++ 2.0 -mm -O -G rebuilds it byte for byte.
 */

#include "xmmblock.h"
#include "xmminit.h"
#include "xmmhand.h"

/* Frees the XMS block an earlier run left allocated, by the handle it saved. */
char FreeStaleXMSBlock(void)
{
	XMSHandle = LoadXmmhand();
	if (IsXMSPresent()) {
		if (!FindXMSDriver())
			return 0;
		if (!UnlockXMS())
			return 0;
		if (!FreeXMS())
			return 0;
	}
	return 1;
}
