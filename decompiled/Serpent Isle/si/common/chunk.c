/* Serpent Isle SI.EXE, resident segment 46 (file offsets 0x020981 to 0x020baf, 558 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

/* path: chunk.c */
#include <string.h>
#include "dosio.h"
#include "easyfile.h"
#include "vooalloc.h"
#include "init.h"
#include "oops.h"
#include "memapi.h"
#include "chunk.h"

#define CHUNK_BYTES 512L

char ChunkFileName[82];
int ChunkFileHandle = -1;

int AllocateChunkBlock(long *block);

inline char IsEmptyBlock(long block) { return block == 0; }

int OpenChunkFile(char *name)
{
	strcpy(ChunkFileName, name);
	ChunkFileHandle = DosOpen(ChunkFileName);
	return ChunkFileHandle != -1;
}

void CloseChunkFile(void)
{
	strcpy(ChunkFileName, "");
	if (ChunkFileHandle >= 0) {
		DosClose(ChunkFileHandle);
		ChunkFileHandle = -1;
	}
}

void ChunkCache::initialize()
{
	int i;

	for (i = 0; i < CACHED_CHUNKS; i++) {
		AllocateChunkBlock(&block[i]);
		if (IsEmptyBlock(block[i]))
			ReportOutOfVoodooMemory();
	}
	id = (int far *) AllocateFarHeap(CACHED_CHUNKS * sizeof(int), 0);
	age = (int far *) AllocateFarHeap(CACHED_CHUNKS * sizeof(int), 0);
	if (id == 0 || age == 0)
		ReportOutOfFarMemory();
}

/* The block holding chunk n, read from the file into the least recently used block if absent. */
long ChunkCache::getChunk(int n)
{
	int slot = -1;
	int i;

	for (i = 0; i < CACHED_CHUNKS; i++)
		if (id[i] == n) {
			slot = i;
			break;
		}
	if (slot < 0) {
		slot = 0;
		for (i = 0; i < CACHED_CHUNKS; i++)
			if (age[i] > age[slot])
				slot = i;
		if (ChunkFileHandle == -1)
			FatalError(__FILE__, 143);
		id[slot] = n;
		ReadHandleToVoodoo(ChunkFileHandle, n * CHUNK_BYTES, CHUNK_BYTES, &block[slot]);
	}
	for (i = 0; i < CACHED_CHUNKS; i++)
		if (age[i] < age[slot])
			age[i]++;
	age[slot] = 0;
	return block[slot];
}

int AllocateChunkBlock(long *block)
{
	*block = AllocateVoodooMemory(&VoodooXmsBlock, CHUNK_BYTES);
	return *block != 0;
}
