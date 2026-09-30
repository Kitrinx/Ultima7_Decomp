#ifndef FRAMEFLG_H
#define FRAMEFLG_H

#include "datanode.h"

struct Flex;
struct CacheEntry;

/* A table of 1024 longs held in voodoo memory, one per type. */
struct FrameFlags : DataNode {
	long data;
	void init();
	char *name();
	void load(char *dir);
	void save(char *dir);
};

extern unsigned char TrimmedShapeBits[128];
extern FrameFlags FrameFlagTable;
extern char *FrameFlagsFileName;

long far GetFlexEntrySize(struct Flex *flex, int index);
char IsShapeTrimmed(CacheEntry *entry);
void far TrimShapeFrames(CacheEntry *entry);

#ifdef __cplusplus
extern "C" {
#endif
long far PackShapeFrames(void far *shape, long keep);
#ifdef __cplusplus
}
#endif

#endif
