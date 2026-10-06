#ifndef U7MANAGE_H
#define U7MANAGE_H

#include "lowlevel.h"

#include "manager.h"

#define FIRST_SPRITE_SHAPE 1024
#define FIRST_FONT_SHAPE 1086
#define FIRST_FACE_SHAPE 1098
#define FIRST_GUMP_SHAPE 1423
#define FIRST_PAPERDOLL_SHAPE 1523
#define FIRST_FILE_BLOCK 1723
#define FIRST_BLOCK 1731
#define SHAPE_HANDLES 1771

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
		DrawTile(cur->address + (frame << 6), x, y);
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
			if (PeekLong(cur->address + (f + 1) * sizeof(long)) == 0)
				ShapeManager_reloadFrame(this, f);
		return cur->address;
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
int far MakeShapeFromImage(int source);

#endif
void SetDataDirectories(void);
#ifdef __cplusplus
}
int far MakeShapeFromImage(int source);

#endif
unsigned ResolveViewHandle(unsigned h);
void OpenShapeManager(void);
int FindShapeRecord(int shape);
void CopyShapeHeader(int to, int from);
void SaveShapeFrameTable(void);

int far GetMaxFrameHeight(long shape, int flags);
int far GetMaxFrameWidth(long shape, int flags);

extern RecordCache ShapeCache;
extern int BlockSlotKeys[40];
extern char *TempFileFormat;
extern char *FacesFileName;
extern char *GumpsFileName;
extern char *PaperdollFileName;
extern char *FontsFileName;
extern char *SpritesFileName;

int far MakeShapeFromImage(int source);

#endif
