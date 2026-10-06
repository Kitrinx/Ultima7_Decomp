/* Serpent Isle SI.EXE, resident segment 159 (file offsets 0x03f46c to 0x03f4ab, 63 bytes).
 * Borland C++ 2.0 -mm -O -G rebuilds it byte for byte.
 * Folder chosen by subsystem.
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
