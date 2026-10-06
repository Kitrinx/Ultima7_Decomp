/* Serpent Isle SI.EXE, resident segment 14 (file offsets 0x010dc3 to 0x011095, 722 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder chosen by subsystem.
 */

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
	int unusedField;
	long block;
	long offset(int i) { return PeekLong(block + (i + 1) * sizeof(long)); }
	int count() { return (offset(0) >> 2) - 1; }
};

unsigned char TrimmedShapeBits[128] = { 0 };
FrameFlags FrameFlagTable;
char *FrameFlagsFileName = "FRAMES.FLG";

void FrameFlags::init()
{
	int i;

	data = AllocateVoodooMemory(&VoodooXmsBlock, 4096L);
	if (data == 0)
		ReportOutOfVoodooMemory();
	for (i = 0; i < 1024; i++) {
		if (i < 150) {  /* the ground tiles */
			PokeLong(data + ((long) i << 2), -1L);
			TrimmedShapeBits[i >> 3] |= 1 << (i & 7);
		} else {
			PokeLong(data + ((long) i << 2), 1L);
			int n = i >> 3;
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
	int handle;

	handle = CreateFileOrFail(BuildPath(dir, FrameFlagsFileName, 0));
	WriteHandleFromVoodoo(handle, -1L, 4096L, data);
	DosClose(handle);
}

void FrameFlags::load(char *dir)
{
	int handle;

	handle = DosOpen(BuildPath(dir, FrameFlagsFileName, 0));
	if (handle != -1) {
		ReadHandleToVoodoo(handle, -1L, 4096L, &data);
		DosClose(handle);
	}
}

long far GetFlexEntrySize(struct FlexOffsetTable *flex, int index)
{
	if (index == flex->count() - 1)
		return PeekLong(flex->block) - flex->offset(index);
	return flex->offset(index + 1) - flex->offset(index);
}

/* 1 when a shape is trimmed or has nothing to trim: a tile, or every frame in use */
char IsShapeTrimmed(CacheEntry *entry)
{
	int i;

	if (entry->id >= 1024 || entry->id < 150 || (char) (~PeekLong(FrameFlagTable.data + ((long) entry->id << 2)) == 0))
		return 1;
	i = entry->id;
	return TrimmedShapeBits[i >> 3] & (1 << (i & 7));
}

void far TrimShapeFrames(CacheEntry *entry)
{
	int n;

	entry->size = PackShapeFrames((void far *)entry->address, FrameFlagTable.data + ((long) entry->id << 2));
	n = entry->id;
	TrimmedShapeBits[n >> 3] |= 1 << (n & 7);
}
