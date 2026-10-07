/* Serpent Isle SI.EXE, resident segment 14 (file offsets 0x010dc3 to 0x011095, 722 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder chosen by subsystem.
 */

#include "u7port.h"
#include <new>
#include "lowlevel.h"
#include "dosio.h"
#include "vooalloc.h"
#include "easyfile.h"
#include "oops.h"
#include "datanode.h"
#include "cacheent.h"
#include "frameflg.h"

/* a table of long offsets at block: the total size, then one offset per record */
struct FlexOffsetTable {
	int16_t unusedField;
	int32_t block;
	int32_t offset(int16_t i) { return PeekLong(block + (i + 1) * sizeof(int32_t)); }
	int16_t count() { return (offset(0) >> 2) - 1; }
};

uint8_t TrimmedShapeBits[128] = { 0 };
FrameFlags FrameFlagTable;
char *const FrameFlagsFileName = "FRAMES.FLG";

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

int32_t GetFlexEntrySize(struct FlexOffsetTable *flex, int16_t index)
{
	if (index == flex->count() - 1)
		return PeekLong(flex->block) - flex->offset(index);
	return flex->offset(index + 1) - flex->offset(index);
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

extern "C" void ResetFrameflgGlobals(void)
{
	memset(TrimmedShapeBits, 0, sizeof(TrimmedShapeBits));
	memset((void *)&FrameFlagTable, 0, sizeof(FrameFlagTable));
}

extern "C" void ConstructFrameflgGlobals(void)
{
	new (&FrameFlagTable) FrameFlags();
}
