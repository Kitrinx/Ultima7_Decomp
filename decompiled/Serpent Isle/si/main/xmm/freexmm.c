/* Serpent Isle SI.EXE, resident segment 153 (file offsets 0x03f078 to 0x03f153, 219 bytes).
 * Borland C++ 2.0 -mm -O -G rebuilds it byte for byte.
 * Folder chosen by subsystem.
 */

/* path: freexmm.c */
#include "init.h"
#include "xmmblock.h"
#include "xmmhand.h"
#include "xmminit.h"
#include "freexmm.h"

#define XMM_ERROR(line) FatalError(__FILE__, line)

void FreeXMM(void)
{
	if (LeaveFlatMode())
		XMM_ERROR(91);
	switch (ExtendedMemoryMethod) {
	case XMM_METHOD_XMS:
		if (XMSPresent)
			FreeXMSBlock();
		break;
	case XMM_METHOD_UNUSED:
		XMM_ERROR(104);
	case XMM_METHOD_INT15:
		ReleaseExtendedMemory();
		break;
	default:
		XMM_ERROR(112);
		break;
	}
}

void ReleaseExtendedMemory(void)
{
	UnhookInt15();
	SetA20Gate(0);
}

void FreeXMSBlock(void)
{
	if (UnlockXMS() == 0)
		XMM_ERROR(180);
	if (FreeXMS() == 0)
		XMM_ERROR(185);
	if (DisableA20Global() == 0)
		XMM_ERROR(191);
	DeleteXmmhand();
}

int ShutdownXMM(void)
{
	FreeXMM();
	return 0;
}
