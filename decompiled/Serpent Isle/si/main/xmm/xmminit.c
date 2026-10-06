/* Serpent Isle SI.EXE, resident segment 155 (file offsets 0x03f154 to 0x03f2a6, 338 bytes).
 * Borland C++ 2.0 -mm -O -G rebuilds it byte for byte.
 * Folder chosen by subsystem.
 */

#include "xmmblock.h"
#include "xmmhand.h"
#include "xmminit.h"

int XMSPresent = 0;
int ExtendedMemoryMethod = 0;
int XMSAlreadyAllocated = 0;
long OverlayMemoryKb = 0;

void far *OpenXMSBlock(void)
{
	if (HasXmmhand() && FreeStaleXMSBlock())
		return 0;
	if ((XMSPresent = IsXMSPresent()) == 0)
		return 0;
	if (FindXMSDriver() == 0)
		return 0;
	if (QueryXMSFree() == 0)
		return 0;
	if (XMSAlreadyAllocated == 0) {
		if ((long) (unsigned) XMSLargestKilobytes > 1024L) {
			OverlayMemoryKb = (long) (unsigned) XMSLargestKilobytes - 1024L;
			XMSLargestKilobytes = 1024;
		}
		if (AllocateXMS(XMSLargestKilobytes) == 0)
			return 0;
		if (LockXMS() == 0) {
			FreeXMS();
			return 0;
		}
	}
	if (EnableA20Global() == 0)
		return 0;
	SaveXmmhand(XMSHandle);
	return XMSBlockAddress;
}

long OpenExtendedMemory(void)
{
	void far *block;

	if (DetectEmsDriver())
		return 0;
	if (IsFlatModeBlocked())
		return 0;
	if ((block = OpenXMSBlock()) == 0) {
		ExtendedMemoryMethod = XMM_METHOD_INT15;
		if ((block = ClaimExtendedMemory()) == 0) {
			LeaveFlatMode();
			return 0;
		}
		SetA20Gate(1);
	} else {
		ExtendedMemoryMethod = XMM_METHOD_XMS;
	}
	return block;
}

long GetXMSBlockSize(void)
{
	return ((long) (unsigned) XMSLargestKilobytes << 10);
}

/* Returns the size in DX:AX, where the call leaves it. */
long GetExtendedMemorySize(void)
{
	GetXMSBlockSize();
}
