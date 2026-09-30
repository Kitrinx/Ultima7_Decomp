#ifndef CHUNK_H
#define CHUNK_H

#define CACHED_CHUNKS 128

/* The most recently used 512-byte chunks of the open file, each in its own block. */
struct ChunkCache {
	long block[CACHED_CHUNKS];
	int far *id;
	int far *age;

	void initialize();
	long getChunk(int n);
};

int OpenChunkFile(char *name);
void CloseChunkFile(void);
int AllocateChunkBlock(long *block);

extern char ChunkFileName[82];
extern int ChunkFileHandle;

#endif
