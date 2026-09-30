/* Black Gate U7.EXE, resident segment 74 (file offsets 0x02ae0b to 0x02b039, 558 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

/* path: ..\common\chunk.c */
#include "u7port.h"
#include <string.h>
#include "dosio.h"
#include "easyfile.h"
#include "vooalloc.h"
#include "init.h"
#include "oops.h"
#include "memapi.h"
#include "chunk.h"

#define CHUNK_BYTES INT32_C(512)

char ChunkFileName[82];
int16_t ChunkFileHandle = -1;

int16_t AllocateChunkBlock(int32_t *block);

inline int8_t IsEmptyBlock(int32_t block) { return block == 0; }

int16_t OpenChunkFile(char *name)
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
	int16_t i;

	for (i = 0; i < CACHED_CHUNKS; i++) {
		AllocateChunkBlock(&block[i]);
		if (IsEmptyBlock(block[i]))
			ReportOutOfVoodooMemory();
	}
	id = (int16_t *) AllocateFarHeap(CACHED_CHUNKS * sizeof(int16_t), 0);
	age = (int16_t *) AllocateFarHeap(CACHED_CHUNKS * sizeof(int16_t), 0);
	if (id == 0 || age == 0)
		ReportOutOfFarMemory();
}

/* The block holding chunk n, read from the file into the least recently used block if absent. */
int32_t ChunkCache::getChunk(int16_t n)
{
	int16_t slot = -1;
	int16_t i;

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

int16_t AllocateChunkBlock(int32_t *block)
{
	*block = AllocateVoodooMemory(&VoodooXmsBlock, CHUNK_BYTES);
	return *block != 0;
}
