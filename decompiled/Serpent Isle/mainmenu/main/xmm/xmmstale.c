/* Serpent Isle MAINMENU.EXE, resident segment 66 (file offsets 0x0192c3 to 0x019302, 63 bytes).
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
