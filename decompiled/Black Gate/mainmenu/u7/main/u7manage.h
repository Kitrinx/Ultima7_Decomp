#ifndef U7MANAGE_H
#define U7MANAGE_H

#include "lowlevel.h"

struct RecordCache;

/* The shape manager's handles: the types (0-1023; below 150 ground tiles), then the records of
 * SPRITES.VGA, FONTS.VGA, FACES.VGA and GUMPS.VGA, eight whole files, and 40 blocks of its own
 * (the first three views). */
#define FIRST_SPRITE_SHAPE  1024
#define FIRST_FONT_SHAPE    1049
#define FIRST_FACE_SHAPE    1059
#define FIRST_GUMP_SHAPE    1359
#define FIRST_FILE_BLOCK    1434
#define FIRST_BLOCK         1442
#define SHAPE_HANDLES       1482

class CacheManager;

/* A block in the memory pool: its handle, where its data sits and how big it is. */
struct CacheEntry {
	int id;
	long data;
	unsigned long size;
	CacheEntry *previous;
	CacheEntry *next;
	CacheEntry *older;
};

/* The memory pool the managers load into. */
struct RecordCache {
	char entryPool[6];
	unsigned long capacity;
	CacheManager *manager;
	unsigned long allocation;
	unsigned long end;
	CacheEntry *firstGap;
	CacheEntry *first;
	CacheEntry *mru;
	int count;
	long used;
	char checking;
	unsigned long blocks() { return count; }
	void charge(long n) { used += n; }
	int percentage() { return 100UL * used / capacity; }
};

void far Cache_touchEntry(RecordCache *, CacheEntry *);
void far Cache_releaseEntry(RecordCache *, CacheEntry *);

class CacheManager {
public:
	RecordCache *own;
	virtual void setSlot(int i, CacheEntry *p) = 0;
	virtual void releaseSlot(int i, CacheEntry *p, unsigned char keep) = 0;
};

/* A table of blocks, each loaded on demand by the derived class. */
class ResourceManager : public CacheManager {
public:
	int count;
	CacheEntry **slots;
	CacheEntry *cur;
	void init(RecordCache *o, int n);
	long checksum(int i);
	void setSlot(int i, CacheEntry *p);
	void releaseSlot(int i, CacheEntry *p, unsigned char keep);
	virtual long load(int i) = 0;
	virtual long locate(int i, int j) = 0;
	unsigned char loaded(int i) { return slots[i] != 0; }
	unsigned char isCurrent() { return cur == own->mru->next; }
	void need(int i) { if (!loaded(i)) load(i); }
	void use(int i)
	{
		need(i);
		cur = slots[i];
		if (cur == 0)
			return;
		if (!isCurrent())
			Cache_touchEntry(own, cur);
	}
	void fetch(int i)
	{
		if (loaded(i))
			return;
		use(i);
	}
	void find(int i)
	{
		if (loaded(i)) {
			cur = slots[i];
		} else {
			use(i);
			cur = slots[i];
		}
	}
	void select(int i)
	{
		if (loaded(i)) {
			cur = slots[i];
			if (!isCurrent())
				Cache_touchEntry(own, cur);
		} else {
			use(i);
			cur = slots[i];
		}
	}
	CacheEntry *entry(int i)
	{
		use(i);
		return slots[i];
	}
	void evict(int i)
	{
		use(i);
		Cache_releaseEntry(own, slots[i]);
	}
	long get(int i)
	{
		need(i);
		cur = slots[i];
		if (cur == 0)
			return 0;
		if (!isCurrent())
			Cache_touchEntry(own, cur);
		return cur->data;
	}
};

struct ShapeTranslations {
	long translations;
	int unusedField1;
	ShapeTranslations() { translations = 0; }
};

class ShapeManager : public ShapeTranslations, public ResourceManager {
public:
	char *directory;
	void init(RecordCache *pool, char *path, int count);
	void ensureFrame(int frame);
	virtual long load(int i) = 0;
	virtual long locate(int i, int j);
	void drawTile(int type, int frame, int x, int y)
	{
		find(type);
		DrawTile(cur->data + (frame << 6), x, y);
	}
};

extern void far ShapeManager_reloadFrame(ShapeManager *, int);

inline unsigned char IsTypeShape(int n) { return n < 1024; }
inline unsigned char IsTileShape(int shape) { return shape < 150; }
inline unsigned char IsObjectShape(int shape) { return shape >= 150 && shape < 1024; }

struct View;
struct Point;

/* The shape manager: the handles listed at the top of this file. */
class U7ShapeManager : public ShapeManager {
public:
	char *names[8];
	char unusedFlag;
	U7ShapeManager();
	long load(int i);
	void releaseSlot(int i, CacheEntry *p, unsigned char keep);
	unsigned char isFrameEmpty(int shape, unsigned f);
	int addFileName(char *name);
	int allocateBlock(long size, int key, char views);
	int reclaimBlock(unsigned key);
	int allocateView(int x0, int y0, int x1, int y1);
	View *lockView(int h);
	void releaseBlock(int h);
	int countFreeSlots(char views);
	unsigned char hasRoomFor(unsigned long n);
	long loadFileBlock(char *name, int h, char flag);
	long loadRecordBlock(char *name, int h, int first);
	void unlockOldBlock();
	void writeBlockFile(char *name, int h, long data, long size);
	void getShapeSize(int *w, int *h, int shape);
	void getFrameSize(int *w, int *h, int shape, int f);
	void saveUnderShape(View *v, int h, int x, int y, int shape, int f);
	void saveUnderRect(int v, int h, int n);
	void restoreUnderShape(View *v, int h, int x, int y, int shape, int f);
	void restoreUnderRect(int v, int h, int n);
	void drawShapeSavingUnder(View *v, int h, int x, int y, int shape, char lit);
	unsigned char isCursorInBounds(int shape, int f, const Point &a, const Point &b);
	long frame(int shape, int f)
	{
		find(shape);
		if (!IsTileShape(shape))
			if (PeekLong(cur->data + (f + 1) * sizeof(long)) == 0)
				ShapeManager_reloadFrame(this, f);
		return cur->data;
	}
};

extern U7ShapeManager gShapeManager;
extern char CurrentDirectory[80];
extern char StaticDirectory[80];
extern char GamedatDirectory[80];
struct CachedFlex;
extern CachedFlex ShapesFile;

/* Shape manager helpers outside the class. */
void far RemapShapeRecord(int, int);
int far ClampShapeFrame(int, int);

void LoadShapesInUse(void);

extern long ShapePoolSize;
extern View ViewBlocks[3];
extern View ViewBlockCopies[3];
extern int ShapeSweepHand;
void ClearBlockTable(int *p);
void ResetViewport(void);
#ifdef __cplusplus
extern "C" {
#endif
void SetDataDirectories(void);
#ifdef __cplusplus
}
#endif
unsigned ResolveViewHandle(unsigned h);
void OpenShapeManager(void);
void CopyShapeHeader(int to, int from);
void SaveShapeFrameTable(void);

int far GetMaxFrameHeight(long shape, int flags);
int far GetMaxFrameWidth(long shape, int flags);

extern RecordCache ShapeCache;
extern int BlockSlotKeys[40];
extern char *TempFileFormat;
extern char *FacesFileName;
extern char *GumpsFileName;
extern char *FontsFileName;
extern char *SpritesFileName;

#endif
