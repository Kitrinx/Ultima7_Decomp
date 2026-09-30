#ifndef CHUNK_H
#define CHUNK_H

#define CACHED_CHUNKS 128

/* The most recently used 512-byte chunks of the open file, each in its own block. */
struct ChunkCache {
	int32_t block[CACHED_CHUNKS];
	int16_t *id;
	int16_t *age;

	void initialize();
	int32_t getChunk(int16_t n);
};

int16_t OpenChunkFile(char *name);
void CloseChunkFile(void);
int16_t AllocateChunkBlock(int32_t *block);

extern char ChunkFileName[82];
extern int16_t ChunkFileHandle;

#endif
