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
	int16_t id;
	int32_t data;
	uint32_t size;
	CacheEntry *previous;
	CacheEntry *next;
	CacheEntry *older;
	CacheEntry *newer;
};

/* The memory pool the managers load into. */
struct RecordCache {
	struct {
		int16_t capacity;
		CacheEntry *entries, *free;
	} entryPool;
	uint32_t capacity;
	CacheManager *manager;
	uint32_t allocation;
	uint32_t end;
	CacheEntry *firstGap;
	CacheEntry *first;
	CacheEntry *mru;
	int16_t count;
	int32_t used;
	int8_t checking;
	uint32_t blocks() { return count; }
	void charge(int32_t n) { used += n; }
	int16_t percentage() { return UINT32_C(100) * used / capacity; }
};

void Cache_touchEntry(RecordCache *, CacheEntry *);
void Cache_releaseEntry(RecordCache *, CacheEntry *);

class CacheManager {
public:
	RecordCache *own;
	virtual void setSlot(int16_t i, CacheEntry *p) = 0;
	virtual void releaseSlot(int16_t i, CacheEntry *p, uint8_t keep) = 0;
};

/* A table of blocks, each loaded on demand by the derived class. */
class ResourceManager : public CacheManager {
public:
	int16_t count;
	CacheEntry **slots;
	CacheEntry *cur;
	void init(RecordCache *o, int16_t n);
	int32_t checksum(int16_t i);
	void setSlot(int16_t i, CacheEntry *p);
	void releaseSlot(int16_t i, CacheEntry *p, uint8_t keep);
	virtual int32_t load(int16_t i) = 0;
	virtual int32_t locate(int16_t i, int16_t j) = 0;
	uint8_t loaded(int16_t i) { return slots[i] != 0; }
	uint8_t isCurrent() { return cur == own->mru->next; }
	void need(int16_t i) { if (!loaded(i)) load(i); }
	void use(int16_t i)
	{
		need(i);
		cur = slots[i];
		if (cur == 0)
			return;
		if (!isCurrent())
			Cache_touchEntry(own, cur);
	}
	void fetch(int16_t i)
	{
		if (loaded(i))
			return;
		use(i);
	}
	void find(int16_t i)
	{
		if (loaded(i)) {
			cur = slots[i];
		} else {
			use(i);
			cur = slots[i];
		}
	}
	void select(int16_t i)
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
	CacheEntry *entry(int16_t i)
	{
		use(i);
		return slots[i];
	}
	void evict(int16_t i)
	{
		use(i);
		Cache_releaseEntry(own, slots[i]);
	}
	int32_t get(int16_t i)
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
	int32_t translations;
	int16_t unusedField1;
	ShapeTranslations() { translations = 0; }
};

class ShapeManager : public ShapeTranslations, public ResourceManager {
public:
	char *directory;
	void init(RecordCache *pool, char *path, int16_t count);
	void ensureFrame(int16_t frame);
	virtual int32_t load(int16_t i) = 0;
	virtual int32_t locate(int16_t i, int16_t j);
	void drawTile(int16_t type, int16_t frame, int16_t x, int16_t y)
	{
		find(type);
		DrawTile(cur->data + (frame << 6), x, y);
	}
};

extern void ShapeManager_reloadFrame(ShapeManager *, int16_t);

inline uint8_t IsTypeShape(int16_t n) { return n < 1024; }
inline uint8_t IsTileShape(int16_t shape) { return shape < 150; }
inline uint8_t IsObjectShape(int16_t shape) { return shape >= 150 && shape < 1024; }

struct View;
struct Point;

/* The shape manager: the handles listed at the top of this file. */
class U7ShapeManager : public ShapeManager {
public:
	char *names[8];
	int8_t unusedFlag;
	U7ShapeManager();
	int32_t load(int16_t i);
	void releaseSlot(int16_t i, CacheEntry *p, uint8_t keep);
	uint8_t isFrameEmpty(int16_t shape, uint16_t f);
	int16_t addFileName(char *name);
	int16_t allocateBlock(int32_t size, int16_t key, int8_t views);
	int16_t reclaimBlock(uint16_t key);
	int16_t allocateView(int16_t x0, int16_t y0, int16_t x1, int16_t y1);
	View *lockView(int16_t h);
	void releaseBlock(int16_t h);
	int16_t countFreeSlots(int8_t views);
	uint8_t hasRoomFor(uint32_t n);
	int32_t loadFileBlock(char *name, int16_t h, int8_t flag);
	int32_t loadRecordBlock(char *name, int16_t h, int16_t first);
	void unlockOldBlock();
	void writeBlockFile(char *name, int16_t h, int32_t data, int32_t size);
	void getShapeSize(int16_t *w, int16_t *h, int16_t shape);
	void getFrameSize(int16_t *w, int16_t *h, int16_t shape, int16_t f);
	void saveUnderShape(View *v, int16_t h, int16_t x, int16_t y, int16_t shape, int16_t f);
	void saveUnderRect(View *v, int16_t h, struct Rect *r);
	void restoreUnderShape(View *v, int16_t h, int16_t x, int16_t y, int16_t shape, int16_t f);
	void restoreUnderRect(View *v, int16_t h, struct Rect *r);
	void drawShapeSavingUnder(View *v, int16_t h, int16_t x, int16_t y, int16_t shape, int8_t lit);
	uint8_t isCursorInBounds(int16_t shape, int16_t f, const Point &a, const Point &b);
	int32_t frame(int16_t shape, int16_t f)
	{
		find(shape);
		if (!IsTileShape(shape))
			if (PeekLong(cur->data + (f + 1) * sizeof(int32_t)) == 0)
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
void RemapShapeRecord(int16_t, int16_t);
int16_t ClampShapeFrame(int16_t, int16_t);

void LoadShapesInUse(void);

extern int32_t ShapePoolSize;
extern View ViewBlocks[3];
extern View ViewBlockCopies[3];
extern int16_t ShapeSweepHand;
void ClearBlockTable(int16_t *p);
void ResetViewport(void);
#ifdef __cplusplus
extern "C" {
#endif
void SetDataDirectories(void);
#ifdef __cplusplus
}
#endif
void OpenShapeManager(void);
void CopyShapeHeader(int16_t to, int16_t from);
void SaveShapeFrameTable(void);

int16_t GetMaxFrameHeight(int32_t shape, int16_t flags);
int16_t GetMaxFrameWidth(int32_t shape, int16_t flags);

extern RecordCache ShapeCache;
extern int16_t BlockSlotKeys[40];
extern char *const TempFileFormat;
extern char *const FacesFileName;
extern char *const GumpsFileName;
extern char *const FontsFileName;
extern char *const SpritesFileName;

#endif
