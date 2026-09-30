/* Black Gate U7.EXE, resident segment 19 (file offsets 0x012629 to 0x0128fb, 722 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Original folder unknown.
 */

#include "u7port.h"
#include "lowlevel.h"
#include "dosio.h"
#include "vooalloc.h"
#include "easyfile.h"
#include "oops.h"
#include "datanode.h"
#include "frameflg.h"
#include "cacheent.h"



uint8_t TrimmedShapeBits[128] = { 0 };
FrameFlags FrameFlagTable;
char *FrameFlagsFileName = "FRAMES.FLG";

void FrameFlags::init()
{
	int16_t i;

	data = AllocateVoodooMemory(&VoodooXmsBlock, INT32_C(4096));
	if (data == 0)
		ReportOutOfVoodooMemory();
	for (i = 0; i < 1024; i++) {
		if (i < 150) {  /* the ground tiles */
			PokeLong(data + ((int32_t) i << 2), -INT32_C(1));
			TrimmedShapeBits[i >> 3] |= 1 << (i & 7);
		} else {
			PokeLong(data + ((int32_t) i << 2), INT32_C(1));
			int16_t n = i >> 3;
			TrimmedShapeBits[n] = TrimmedShapeBits[n] & ~(1 << (i & 7));
		}
	}
}

char *FrameFlags::name()
{
	return FrameFlagsFileName;
}

void FrameFlags::save(char *dir)
{
	int16_t handle;

	handle = CreateFileOrFail(BuildPath(dir, FrameFlagsFileName, 0));
	WriteHandleFromVoodoo(handle, -INT32_C(1), INT32_C(4096), data);
	DosClose(handle);
}

void FrameFlags::load(char *dir)
{
	int16_t handle;

	handle = DosOpen(BuildPath(dir, FrameFlagsFileName, 0));
	if (handle != -1) {
		ReadHandleToVoodoo(handle, -INT32_C(1), INT32_C(4096), &data);
		DosClose(handle);
	}
}

/* 1 when a shape is trimmed or has nothing to trim: a tile, or every frame in use */
int8_t IsShapeTrimmed(CacheEntry *entry)
{
	int16_t i;

	if (entry->id >= 1024 || entry->id < 150 || (int8_t) (~PeekLong(FrameFlagTable.data + ((int32_t) entry->id << 2)) == 0))
		return 1;
	i = entry->id;
	return TrimmedShapeBits[i >> 3] & (1 << (i & 7));
}

void TrimShapeFrames(CacheEntry *entry)
{
	int16_t n;

	entry->size = PackShapeFrames(entry->address, FrameFlagTable.data + ((int32_t) entry->id << 2));
	n = entry->id;
	TrimmedShapeBits[n >> 3] |= 1 << (n & 7);
}
