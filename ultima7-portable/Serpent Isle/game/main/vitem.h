#ifndef VITEM_H
#define VITEM_H

struct IfixCache;
struct objref;

struct WorldView;
struct Coord;

void InitIfixCaches(void);
IfixCache *GetIfixCache(uint8_t n);
void LoadChunkItems(Coord x, Coord y, WorldView *map);
void LoadWindowChunks(Coord ox, Coord oy, Coord nx, Coord ny, WorldView *view);
void RemoveContents(objref container);
void UnloadChunk(Coord x, Coord y);
void UnloadWindowChunks(Coord ox, Coord oy, Coord nx, Coord ny);

extern char *IfixFileFormat;
extern char *IfixFilePattern;
extern IfixCache IfixCaches[4];
int32_t GetLargestIfixSize(void);

#endif
