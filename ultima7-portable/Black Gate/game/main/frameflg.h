#ifndef FRAMEFLG_H
#define FRAMEFLG_H

#include "datanode.h"

struct CacheEntry;

/* A table of 1024 longs held in voodoo memory, one per type. */
struct FrameFlags : DataNode {
	int32_t data;
	void init();
	char *name();
	void load(char *dir);
	void save(char *dir);
};

extern uint8_t TrimmedShapeBits[128];
extern FrameFlags FrameFlagTable;
extern char *const FrameFlagsFileName;

int8_t IsShapeTrimmed(CacheEntry *entry);
void TrimShapeFrames(CacheEntry *entry);

#ifdef __cplusplus
extern "C" {
#endif
int32_t PackShapeFrames(int32_t shape, int32_t keep);
#ifdef __cplusplus
}
#endif

#endif
