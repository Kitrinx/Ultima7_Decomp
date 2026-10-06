/* Serpent Isle SI.EXE, resident segment 36 (file offsets 0x01a354 to 0x01c347, 8179 bytes).
 * Borland C++ 2.0 -mm -O -G -P -d -b- rebuilds it byte for byte as C++.
 */

/* path: u7manage.c */
#include <dir.h>
#include <stdio.h>
#include <mem.h>
#include "lowlevel.h"
#include "dosio.h"
#include "flxcach.h"
#include "easyfile.h"
#include "colbuf.h"
#include "init.h"
#include "bltshape.h"
#include "debug.h"
#include "chkfile.h"
#include "rescache.h"
#include "oops.h"
#include "xmmblock.h"
#include "frameflg.h"
#include "u7manage.h"

struct Rect {
	int x0, y0, x1, y1;
	void set(int a, int b, int c, int d) { x0 = a; y0 = b; x1 = c; y1 = d; }
};

#define CLEAR_RECT(x)   do { Rect *r_ = &(x); r_->x0 = 0; r_->y0 = 0; r_->x1 = 0; r_->y1 = 0; } while (0)

struct ViewId {
	int id;
	ViewId() { id = 0; }
};

struct ViewPixels {
	long rowTable;
	ViewPixels() { rowTable = 0; }
};

/* A drawing area: its handle, where its pixels start and the rectangle it covers. */
struct View : ViewId, ViewPixels {
	Rect clip;
	View() { CLEAR_RECT(clip); }
};

/* Slots 0-2 of the table hold views, slots 3-39 blocks; each keeps a 15-bit key and a flag
 * marking the block discardable. Key 0x7fff marks an anonymous block. */
extern int BlockSlotKeys[];

inline unsigned GetSlotKey(int i) { return BlockSlotKeys[i] & 0x7fff; }
inline unsigned char IsSlotInUse(int i) { return GetSlotKey(i) != 0; }
inline unsigned char IsAnonymousSlot(int i) { return GetSlotKey(i) == 0x7fff; }
inline unsigned char IsDiscardable(int i) { return (BlockSlotKeys[i] & 0x8000) == 0x8000; }

RecordCache ShapeCache;
int BlockSlotKeys[40];
extern View ScreenView;
extern View Viewport;

extern "C" char far TestShapeHit(long, int, const Point far *, const Point far *, int);
extern unsigned char far GetShapeFrameKind(unsigned type);

/* Where type i's entry sits in the frame flag table. */
#define OFFSET(i)   (FrameFlagTable.data + ((long) (i) << 2))

char *TempFileFormat = "TEMP%04x";
char *FacesFileName = "FACES.VGA";
char *GumpsFileName = "GUMPS.VGA";
char *PaperdollFileName = "PAPERDOL.VGA";
char *FontsFileName = "FONTS.VGA";
char *SpritesFileName = "SPRITES.VGA";
U7ShapeManager gShapeManager;
char CurrentDirectory[80] = ".";
char StaticDirectory[80] = ".";
char GamedatDirectory[80] = ".";
CachedFlex ShapesFile;
long ShapePoolSize = 583680L;
View ViewBlocks[3];
View ViewBlockCopies[3];
/* The first 1024 loads only count up; then a hand sweeps the object types. */
int ShapeSweepHand = -1024;

void ClearBlockTable(int *p)
{
	int i;

	for (i = 0; i < 40; i++)
		p[i] = 0;
}

void ResetViewport(void)
{
	Viewport = ScreenView;
}

extern "C" void SetDataDirectories(void)
{
	if (getcwd(CurrentDirectory, 80) == 0)
		ReportFileNotFound(CurrentDirectory);
	else {
		sprintf(StaticDirectory, "%s\\STATIC", CurrentDirectory);
		StaticPath = StaticDirectory;
		sprintf(GamedatDirectory, "%s\\GAMEDAT", CurrentDirectory);
		GamedatPath = GamedatDirectory;
	}
}

/* True when frame f of the shape is past its last or has no pixels. */
unsigned char U7ShapeManager::isFrameEmpty(int shape, unsigned f)
{
	long offset;
	int width;

	if (ShapeManager_getFrameCount(this, shape) <= f)
		return 1;
	offset = PeekLong(frame(shape, f) + (f + 1) * sizeof(long));
	width = PeekWord(cur->address + offset + 8);
	return width == 0;
}

int U7ShapeManager::addFileName(char *name)
{
	int i;

	for (i = 0; i < 8 && names[i] != 0; i++)
		;
	if (i >= 8)
		ReportError(0xe400);
	ReplaceString(&names[i], name);
	return i + FIRST_FILE_BLOCK;
}

/* Claims a slot for key and gives it a block of size bytes; slots 0-2 serve views. The eviction
 * search starts at last, so a full table always ends in error 0xe401. */
int U7ShapeManager::allocateBlock(long size, int key, char views)
{
	int first = views ? 0 : 3;
	int last = views ? 3 : 40;
	int i = last;

	if (key == 0)
		key = 0x7fff;
	if (i == last)
		for (i = first; i < last && IsSlotInUse(i); i++)
			;
	if (i == last) {
		for (; i < last && !IsDiscardable(i); i++)
			;
		if (i < last) {
			evict(i + FIRST_BLOCK);
			BlockSlotKeys[i] = 0;
		} else
			ReportError(0xe401);
	}
	int h = i + FIRST_BLOCK;
	Cache_allocateAddress(own, size, h);
	BlockSlotKeys[i] = key;
	return h;
}

/* Reclaims the slot holding key. */
int U7ShapeManager::reclaimBlock(unsigned key)
{
	int first = 3;
	int last = 40;
	int i = last;

	if (key != 0x7fff) {
		for (i = first; i < last && (BlockSlotKeys[i] & 0x7fff) != key; i++)
			;
		if (i != last) {
			BlockSlotKeys[i] &= 0x7fff;
			return i + FIRST_BLOCK;
		}
	}
	return 0;
}

/* Claims a slot sized for the rectangle x0,y0 to x1,y1 and its row table. */
int U7ShapeManager::allocateView(int x0, int y0, int x1, int y1)
{
	long size;
	int h;
	int idx;

	size = ((long) (x1 - x0) + 1) * ((long) (y1 - y0) + 1) + (((long) (y1 - y0) + 1) << 2);
	h = allocateBlock(size, 0x7fff, 1);
	idx = h - FIRST_BLOCK;
	if (idx >= 3)
		AssertFail(__FILE__, 364);
	ViewBlocks[idx].clip.set(x0, y0, x1, y1);
	ViewBlocks[idx].id = 0;
	ViewBlocks[idx].rowTable = 0;
	memcpy(&ViewBlockCopies[idx], &ViewBlocks[idx], sizeof(View));
	return h;
}

/* Lays the rectangle of view h out row by row in its block. */
View *U7ShapeManager::lockView(int h)
{
	int idx = h - FIRST_BLOCK;

	if (idx < 0 || idx > 3)
		AssertFail(__FILE__, 389);
	int x0 = ViewBlocks[idx].clip.x0;
	int y0 = ViewBlocks[idx].clip.y0;
	long width = ViewBlocks[idx].clip.x1 - x0 + 1;
	long rows = ViewBlocks[idx].clip.y1 - y0 + 1;
	long offset = 0;
	long start = get(h);

	ViewBlocks[idx].rowTable = start + width * rows;
	ViewBlockCopies[idx].rowTable = start + width * rows;
	for (unsigned i = 0; i < rows; i++) {
		SetRowAddress(i + y0, start + offset + x0, ViewBlocks[idx].rowTable);
		offset += width;
	}
	return &ViewBlockCopies[idx];
}

/* A handle of one of the three views stands for the view itself. */
unsigned ResolveViewHandle(unsigned h)
{
	if (h < FIRST_BLOCK + 3)
		return (unsigned) gShapeManager.lockView(h);
	return h;
}

void U7ShapeManager::releaseBlock(int h)
{
	if (h >= FIRST_FILE_BLOCK && h < FIRST_BLOCK) {
		int i = h - FIRST_FILE_BLOCK;

		if (names[i] != 0)
			delete names[i];
		names[i] = 0;
	} else if (h >= FIRST_BLOCK && h < SHAPE_HANDLES) {
		int k = h - FIRST_BLOCK;

		BlockSlotKeys[k] |= 0x8000;
		if (IsAnonymousSlot(k))
			evict(h);
	}
}

/* How many slots of the range are entirely free. */
int U7ShapeManager::countFreeSlots(char views)
{
	int n = 0;
	int i = views ? 0 : 3;
	int last = views ? 3 : 40;

	for (; i < last; i++)
		if (!IsSlotInUse(i) && !IsDiscardable(i))
			n++;
	return n;
}

/* Whether n more bytes fit in the pool beside the blocks the table holds. */
unsigned char U7ShapeManager::hasRoomFor(unsigned long n)
{
	unsigned long total = 0;
	int i;

	for (i = 0; i < 40; i++)
		if (IsSlotInUse(i))
			total += entry(i + FIRST_BLOCK)->size;
	return total + n < own->capacity - 10240;
}

struct LoadFile : DataFile { LoadFile() : DataFile() {} };

/* Loads a whole file into a new block for handle h. */
long U7ShapeManager::loadFileBlock(char *name, int h, char flag)
{
	long block = 0;
	long size;
	LoadFile f;

	if (!f.open(name, 1))
		ReportFileNotFound(name);
	size = f.getLength();
	block = Cache_allocateAddress(own, size, h);
	ReadFileToVoodoo(&f, block, size);
	return block;
}

/* Loads record h - first of a record file into a new block for handle h. */
long U7ShapeManager::loadRecordBlock(char *name, int h, int first)
{
	struct FlexEntry s;
	long block;
	Flex f;

	if (!f.open(name))
		ReportFileNotFound(name);
	f.getEntry(h - first, &s);
	if (s.empty()) {
		return 0;
	}
	block = Cache_allocateAddress(own, s.size, h);
	f.readEntryToVoodoo(&s, block, 0);
	f.close();
	return block;
}

/* Passes over the oldest five sixths of the pool's trimmed blocks and trims the first untrimmed one. */
void U7ShapeManager::unlockOldBlock()
{
	CacheEntry *p = Cache_getOldestEntry(own);
	int n = (own->blocks() - 2) * 5 / 6;

	while (IsShapeTrimmed(p) && n != 0) {
		p = p->older;
		n--;
	}
	if (n == 0)
		return;
	own->charge(-p->size);
	TrimShapeFrames(p);
	own->charge(p->size);
}

/* Loads handle i; object types first make room by freeing released and old blocks. */
long U7ShapeManager::load(int i)
{
	long r = 0;

	if (i < 0)
		ReportError(0xe407);
	if (IsTypeShape(i)) {
		if (IsObjectShape(i)) {
			CacheEntry *v;

			do {
				v = Cache_findReleased(own);
				if (v != 0)
					Cache_freeEntry(own, &v);
			} while (v != 0);
			if (own->blocks() > 20)
				unlockOldBlock();
		}
		struct FlexEntry s;
		int n;

		ShapesFile.getEntry(i, &s);
		r = Cache_allocateAddress(own, s.size, i);
		ShapesFile.readEntryToVoodoo(&s, r, 0);
		if (IsObjectShape(i)) {
			switch (GetShapeFrameKind(i)) {
			case 0:
				SetFlatBit(OFFSET(i), 0);
				break;
			case 1:
				PokeLong(OFFSET(i), -1L);
				break;
			case 2:
				PokeLong(OFFSET(i), 0x70007L);
				break;
			}
			n = i >> 3;
			TrimmedShapeBits[n] = TrimmedShapeBits[n] & ~(1 << (i & 7));
		}
		if (ShapeSweepHand < 0)
			ShapeSweepHand++;
		else {
			do
				ShapeSweepHand = (ShapeSweepHand + 1) % (1024 - 150);
			while (loaded(ShapeSweepHand + 150));
			int n = i >> 3;
			TrimmedShapeBits[n] = TrimmedShapeBits[n] & ~(1 << (i & 7));
			PokeLong(OFFSET(i), 1L);
		}
	} else if (i >= FIRST_FILE_BLOCK && i < FIRST_BLOCK)
		r = loadFileBlock(names[i - FIRST_FILE_BLOCK], i, 1);
	else if (i >= FIRST_BLOCK && i < SHAPE_HANDLES) {
		int k = i - FIRST_BLOCK;

		if (!IsSlotInUse(k))
			ReportErrorSubtype(0xe409, k);
		r = loadFileBlock(BuildNumberedTempPath(CurrentDirectory, TempFileFormat, k, 1), i, 1);
	} else if (i >= FIRST_SPRITE_SHAPE && i < FIRST_FONT_SHAPE)
		r = loadRecordBlock(BuildPath(StaticPath, SpritesFileName, 0), i, FIRST_SPRITE_SHAPE);
	else if (i >= FIRST_FONT_SHAPE && i < FIRST_FACE_SHAPE)
		r = loadRecordBlock(BuildPath(StaticPath, FontsFileName, 0), i, FIRST_FONT_SHAPE);
	else if (i >= FIRST_FACE_SHAPE && i < FIRST_GUMP_SHAPE) {
		r = loadRecordBlock(BuildPath(StaticPath, FacesFileName, 0), i, FIRST_FACE_SHAPE);
		if (r == 0)
			r = loadRecordBlock(BuildPath(StaticPath, FacesFileName, 0), 0, FIRST_FACE_SHAPE);
	}
	else if (i >= FIRST_GUMP_SHAPE && i < FIRST_PAPERDOLL_SHAPE)
		r = loadRecordBlock(BuildPath(StaticPath, GumpsFileName, 0), i, FIRST_GUMP_SHAPE);
	else if (i >= FIRST_PAPERDOLL_SHAPE && i < FIRST_FILE_BLOCK)
		r = loadRecordBlock(BuildPath(StaticPath, PaperdollFileName, 0), i, FIRST_PAPERDOLL_SHAPE);
	return r;
}

/* Writes a block out to the file name. */
void U7ShapeManager::writeBlockFile(char *name, int h, long data, long size)
{
	unsigned char ok = 0;
	int handle;

	handle = DosCreate(name);
	if (handle >= 0) {
		WriteHandleFromVoodoo(handle, 0L, size, data);
		ok = 1;
		DosClose(handle);
	}
	if (!ok)
		ReportErrorSubtype(0xe406, h);
}

/* The pool drops block h: a block still in use is saved to a file first. */
void U7ShapeManager::releaseSlot(int h, CacheEntry *p, unsigned char keep)
{
	int k;

	if (h >= FIRST_BLOCK && h < SHAPE_HANDLES) {
		if (!keep) {
			k = h - FIRST_BLOCK;
			if (!IsDiscardable(k))
				writeBlockFile(BuildNumberedTempPath(CurrentDirectory, TempFileFormat, k, 1), h, p->address,
					p->size);
			else
				BlockSlotKeys[k] = 0;
		} else {
			if (!IsDiscardable(h - FIRST_BLOCK))
				goto done;
			BlockSlotKeys[h - FIRST_BLOCK] = 0;
		}
	}
done:
	ResourceManager::releaseSlot(h, p, keep);
}

U7ShapeManager::U7ShapeManager()
{
	int i;

	for (i = 0; i < 8; i++)
		names[i] = 0;
	ClearBlockTable(BlockSlotKeys);
	unusedFlag = 0;
}

/* The width and height of a shape's largest frame. */
void U7ShapeManager::getShapeSize(int *w, int *h, int shape)
{
	if (IsTileShape(shape)) {
		*w = 8;
		*h = 8;
		return;
	}
	find(shape);
	if (IsObjectShape(shape)) {
		CacheEntry *e = cur;

		if (IsShapeTrimmed(e)) {
			Cache_freeEntry(own, &cur);
			use(shape);
		}
	}
	*w = GetMaxFrameWidth(get(shape), 1);
	*h = GetMaxFrameHeight(get(shape), 1);
}

/* The width and height of frame f of the shape. */
void U7ShapeManager::getFrameSize(int *w, int *h, int shape, int f)
{
	Rect r, *p = &r;

	p->x0 = 0;
	p->y0 = 0;
	p->x1 = 0;
	p->y1 = 0;
	GetFrameBounds(&r, 0, 0, frame(shape, f), f, 1);
	*w = r.x1 - r.x0 + 1;
	*h = r.y1 - r.y0 + 1;
}

void U7ShapeManager::saveUnderShape(View *v, int h, int x, int y, int shape, int f)
{
	SaveUnderFrame(v, get(h), x, y, frame(shape, f), f, 0x101);
}

void U7ShapeManager::saveUnderRect(int v, int h, int n)
{
	SaveRect(v, get(h), (void *)n, 1);
}

void U7ShapeManager::restoreUnderShape(View *v, int h, int x, int y, int shape, int f)
{
	RestoreUnderFrame(v, get(h), x, y, frame(shape, f), f, 0x101);
}

void U7ShapeManager::restoreUnderRect(int v, int h, int n)
{
	RestoreRect(v, get(h), (void *)n, 1);
}

/* Draws a shape; one with more than one frame uses its second when lit. */
void U7ShapeManager::drawShapeSavingUnder(View *v, int h, int x, int y, int shape, char lit)
{
	int f = ShapeManager_getFrameCount(this, shape) == 1 ? 0 : 1;

	saveUnderShape(v, h, x, y, shape, f);
	ShapeManager_draw(this, v, x, y, shape, lit ? f : 0, 0, 0);
}

unsigned char U7ShapeManager::isCursorInBounds(int shape, int f, const Point &a, const Point &b)
{
	return TestShapeHit(frame(shape, f), f, &a, &b, 1);
}

/* Opens the shapes file and sets up the pool and the shape manager. */
void OpenShapeManager(void)
{
	FlexEntry s;

	ShapesFile.open(BuildPath(StaticPath, ShapesFileName, 0));
	if (!ShapesFile.isopen())
		ReportFileNotFound(BuildPath(StaticPath, ShapesFileName, 0));
	ShapesFile.getEntry(0, &s);
	Cache_initialize(&ShapeCache, &gShapeManager, ShapePoolSize, 250);
	gShapeManager.init(&ShapeCache, StaticPath, SHAPE_HANDLES);
	gShapeManager.own->checking = 0;
}

int FindShapeRecord(int shape)
{
	FlexEntry entry, other;
	int i;

	ShapesFile.getEntry(shape, &entry);
	for (i = 0; i < 1200; i++) {
		ShapesFile.Flex::getEntry(i, &other);
		if ((unsigned char)(other == entry))
			return i;
	}
}

/* Rewrites the header of a shape from record n of the file. */
void RemapShapeRecord(int shape, int n)
{
	FlexEntry s;
	unsigned char was = gShapeManager.loaded(shape);

	if (was) {
		gShapeManager.use(shape);
		Cache_freeEntry(&ShapeCache, &gShapeManager.slots[shape]);
	}
	ShapesFile.Flex::getEntry(n, &s);
	ShapesFile.store(shape, &s);
	if (was)
		gShapeManager.use(shape);
}

/* Copies the header of record from to record to. */
void CopyShapeHeader(int to, int from)
{
	FlexEntry s;

	ShapesFile.getEntry(from, &s);
	ShapesFile.writeEntry(to, &s);
}

int ClampShapeFrame(int shape, int frame)
{
	return ClampInt(0, frame, ShapeManager_getFrameCount(&gShapeManager, shape) - 1);
}

void SaveShapeFrameTable(void)
{
	FrameFlagTable.save(CurrentDirectory);
}

/* Loads every type the frame table marks as in use. */
void LoadShapesInUse(void)
{
	int i;

	FrameFlagTable.load(GamedatPath);
	for (i = 0; i < 1024; i++) {
		if (PeekLong(OFFSET(i)) == -1L || PeekLong(OFFSET(i)) == 1L)
			continue;
		gShapeManager.use(i);
	}
}
