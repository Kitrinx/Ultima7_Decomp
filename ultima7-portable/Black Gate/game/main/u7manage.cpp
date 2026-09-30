/* Black Gate U7.EXE, resident segment 60 (file offsets 0x022e85 to 0x024dab, 7974 bytes).
 * Borland C++ 2.0 -mm -O -G -P -d -b- rebuilds it byte for byte as C++.
 */

/* path: u7manage.c */
#include "u7port.h"
#include <stdio.h>
#include "lowlevel.h"
#include "dosio.h"
#include "flxcach.h"
#include "easyfile.h"
#include "colbuf.h"
#include "init.h"
#include "voice.h"
#include "bltshape.h"
#include "debug.h"
#include "preload.h"
#include "chkfile.h"
#include "rescache.h"
#include "oops.h"
#include "xmmblock.h"
#include "frameflg.h"
#include "u7manage.h"

struct Rect {
	int16_t x0, y0, x1, y1;
	void set(int16_t a, int16_t b, int16_t c, int16_t d) { x0 = a; y0 = b; x1 = c; y1 = d; }
};

#define CLEAR_RECT(x)   do { Rect *r_ = &(x); r_->x0 = 0; r_->y0 = 0; r_->x1 = 0; r_->y1 = 0; } while (0)

struct ViewId {
	int16_t id;
	ViewId() { id = 0; }
};

struct ViewPixels {
	int32_t rowTable;
	ViewPixels() { rowTable = 0; }
};

/* A drawing area: its handle, where its pixels start and the rectangle it covers. */
struct View : ViewId, ViewPixels {
	Rect clip;
	View() { CLEAR_RECT(clip); }
};

/* Slots 0-2 of the table hold views, slots 3-39 blocks; each keeps a 15-bit key and a flag
 * marking the block discardable. Key 0x7fff marks an anonymous block. */
extern int16_t BlockSlotKeys[];

inline uint16_t GetSlotKey(int16_t i) { return BlockSlotKeys[i] & 0x7fff; }
inline uint8_t IsSlotInUse(int16_t i) { return GetSlotKey(i) != 0; }
inline uint8_t IsAnonymousSlot(int16_t i) { return GetSlotKey(i) == 0x7fff; }
inline uint8_t IsDiscardable(int16_t i) { return (BlockSlotKeys[i] & 0x8000) == 0x8000; }

RecordCache ShapeCache;
int16_t BlockSlotKeys[40];
extern "C" View ScreenView;
extern View Viewport;

extern "C" int8_t TestShapeHit(int32_t, int16_t, const Point *, const Point *, int16_t);
extern uint8_t GetShapeFrameKind(uint16_t type);

/* Where type i's entry sits in the frame flag table. */
#define OFFSET(i)   (FrameFlagTable.data + ((int32_t) (i) << 2))

char *TempFileFormat = "TEMP%04x";
char *FacesFileName = "FACES.VGA";
char *GumpsFileName = "GUMPS.VGA";
char *FontsFileName = "FONTS.VGA";
char *SpritesFileName = "SPRITES.VGA";
U7ShapeManager gShapeManager;
char CurrentDirectory[80] = ".";
char StaticDirectory[80] = ".";
char GamedatDirectory[80] = ".";
CachedFlex ShapesFile;
int32_t ShapePoolSize = INT32_C(583680);
View ViewBlocks[3];
View ViewBlockCopies[3];
/* The first 1024 loads only count up; then a hand sweeps the object types. */
int16_t ShapeSweepHand = -1024;

void ClearBlockTable(int16_t *p)
{
	int16_t i;

	for (i = 0; i < 40; i++)
		p[i] = 0;
}

void ResetViewport(void)
{
	Viewport = ScreenView;
}

extern "C" void SetDataDirectories(void)
{
	strcpy(StaticDirectory, "STATIC");
	StaticPath = StaticDirectory;
	strcpy(GamedatDirectory, "GAMEDAT");
	GamedatPath = GamedatDirectory;
}

/* True when frame f of the shape is past its last or has no pixels. */
uint8_t U7ShapeManager::isFrameEmpty(int16_t shape, uint16_t f)
{
	int32_t offset;
	int16_t width;

	if ((uint16_t)(ShapeManager_getFrameCount(this, shape)) <= f)
		return 1;
	offset = PeekLong(frame(shape, f) + (f + 1) * sizeof(int32_t));
	width = PeekWord(cur->data + offset + 8);
	return width == 0;
}

int16_t U7ShapeManager::addFileName(char *name)
{
	int16_t i;

	for (i = 0; i < 8 && names[i] != 0; i++)
		;
	if (i >= 8)
		ReportError(0xe400);
	ReplaceString(&names[i], name);
	return i + FIRST_FILE_BLOCK;
}

/* Claims a slot for key and gives it a block of size bytes; slots 0-2 serve views. The eviction
 * search starts at last, so a full table always ends in error 0xe401. */
int16_t U7ShapeManager::allocateBlock(int32_t size, int16_t key, int8_t views)
{
	int16_t first = views ? 0 : 3;
	int16_t last = views ? 3 : 40;
	int16_t i = last;

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
	int16_t h = i + FIRST_BLOCK;
	Cache_allocateAddress(own, size, h);
	BlockSlotKeys[i] = key;
	return h;
}

/* Reclaims the slot holding key. */
int16_t U7ShapeManager::reclaimBlock(uint16_t key)
{
	int16_t first = 3;
	int16_t last = 40;
	int16_t i = last;

	if (key != 0x7fff) {
		for (i = first; i < last && ((uint16_t)(BlockSlotKeys[i] & 0x7fff)) != key; i++)
			;
		if (i != last) {
			BlockSlotKeys[i] &= 0x7fff;
			return i + FIRST_BLOCK;
		}
	}
	return 0;
}

/* Claims a slot sized for the rectangle x0,y0 to x1,y1 and its row table. */
int16_t U7ShapeManager::allocateView(int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
	int32_t size;
	int16_t h;
	int16_t idx;

	size = ((int32_t) (x1 - x0) + 1) * ((int32_t) (y1 - y0) + 1) + (((int32_t) (y1 - y0) + 1) << 2);
	h = allocateBlock(size, 0x7fff, 1);
	idx = h - FIRST_BLOCK;
	if (idx >= 3)
		AssertFail(__FILE__, 360);
	ViewBlocks[idx].clip.set(x0, y0, x1, y1);
	ViewBlocks[idx].id = 0;
	ViewBlocks[idx].rowTable = 0;
	memcpy(&ViewBlockCopies[idx], &ViewBlocks[idx], sizeof(View));
	return h;
}

/* Lays the rectangle of view h out row by row in its block. */
View *U7ShapeManager::lockView(int16_t h)
{
	int16_t idx = h - FIRST_BLOCK;

	if (idx < 0 || idx > 3)
		AssertFail(__FILE__, 384);
	int16_t x0 = ViewBlocks[idx].clip.x0;
	int16_t y0 = ViewBlocks[idx].clip.y0;
	int32_t width = ViewBlocks[idx].clip.x1 - x0 + 1;
	int32_t rows = ViewBlocks[idx].clip.y1 - y0 + 1;
	int32_t offset = 0;
	int32_t start = get(h);

	ViewBlocks[idx].rowTable = start + width * rows;
	ViewBlockCopies[idx].rowTable = start + width * rows;
	for (uint16_t i = 0; i < rows; i++) {
		SetRowAddress(i + y0, start + offset + x0, ViewBlocks[idx].rowTable);
		offset += width;
	}
	return &ViewBlockCopies[idx];
}

void U7ShapeManager::releaseBlock(int16_t h)
{
	if (h >= FIRST_FILE_BLOCK && h < FIRST_BLOCK) {
		int16_t i = h - FIRST_FILE_BLOCK;

		if (names[i] != 0)
			delete names[i];
		names[i] = 0;
	} else if (h >= FIRST_BLOCK && h < SHAPE_HANDLES) {
		int16_t k = h - FIRST_BLOCK;

		BlockSlotKeys[k] |= 0x8000;
		if (IsAnonymousSlot(k))
			evict(h);
	}
}

/* How many slots of the range are entirely free. */
int16_t U7ShapeManager::countFreeSlots(int8_t views)
{
	int16_t n = 0;
	int16_t i = views ? 0 : 3;
	int16_t last = views ? 3 : 40;

	for (; i < last; i++)
		if (!IsSlotInUse(i) && !IsDiscardable(i))
			n++;
	return n;
}

/* Whether n more bytes fit in the pool beside the blocks the table holds. */
uint8_t U7ShapeManager::hasRoomFor(uint32_t n)
{
	uint32_t total = 0;
	int16_t i;

	for (i = 0; i < 40; i++)
		if (IsSlotInUse(i))
			total += entry(i + FIRST_BLOCK)->size;
	return total + n < own->capacity - 10240;
}

struct LoadFile : DataFile { LoadFile() : DataFile() {} };

/* Loads a whole file into a new block for handle h. */
int32_t U7ShapeManager::loadFileBlock(char *name, int16_t h, int8_t flag)
{
	int32_t block = 0;
	int32_t size;
	LoadFile f;

	if (!f.open(name, 1))
		ReportFileNotFound(name);
	size = f.getLength();
	block = Cache_allocateAddress(own, size, h);
	ReadFileToVoodoo(&f, block, size);
	return block;
}

/* Loads record h - first of a record file into a new block for handle h. */
int32_t U7ShapeManager::loadRecordBlock(char *name, int16_t h, int16_t first)
{
	struct FlexEntry s;
	int32_t block;
	Flex f;

	if (!f.open(name))
		ReportFileNotFound(name);
	f.getEntry(h - first, &s);
	block = Cache_allocateAddress(own, s.size, h);
	f.readEntryToVoodoo(&s, block, 0);
	f.close();
	return block;
}

/* Passes over the oldest five sixths of the pool's trimmed blocks and trims the first untrimmed one. */
void U7ShapeManager::unlockOldBlock()
{
	CacheEntry *p = Cache_getOldestEntry(own);
	int16_t n = (own->blocks() - 2) * 5 / 6;

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
int32_t U7ShapeManager::load(int16_t i)
{
	int32_t r = 0;

	SpeechPlayer.continuePlaying();
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
		int16_t n;

		ShapesFile.getEntry(i, &s);
		r = Cache_allocateAddress(own, s.size, i);
		ShapesFile.readEntryToVoodoo(&s, r, 0);
		if (IsObjectShape(i)) {
			switch (GetShapeFrameKind(i)) {
			case 0:
				SetFlatBit(OFFSET(i), 0);
				break;
			case 1:
				PokeLong(OFFSET(i), -INT32_C(1));
				break;
			case 2:
				PokeLong(OFFSET(i), INT32_C(0x70007));
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
			int16_t n = i >> 3;
			TrimmedShapeBits[n] = TrimmedShapeBits[n] & ~(1 << (i & 7));
			PokeLong(OFFSET(i), INT32_C(1));
		}
	} else if (i >= FIRST_FILE_BLOCK && i < FIRST_BLOCK)
		r = loadFileBlock(names[i - FIRST_FILE_BLOCK], i, 1);
	else if (i >= FIRST_BLOCK && i < SHAPE_HANDLES) {
		int16_t k = i - FIRST_BLOCK;

		if (!IsSlotInUse(k))
			ReportErrorSubtype(0xe409, k);
		r = loadFileBlock(BuildNumberedTempPath(CurrentDirectory, TempFileFormat, k, 1), i, 1);
	} else if (i >= FIRST_SPRITE_SHAPE && i < FIRST_FONT_SHAPE)
		r = loadRecordBlock(BuildPath(StaticPath, SpritesFileName, 0), i, FIRST_SPRITE_SHAPE);
	else if (i >= FIRST_FONT_SHAPE && i < FIRST_FACE_SHAPE)
		r = loadRecordBlock(BuildPath(StaticPath, FontsFileName, 0), i, FIRST_FONT_SHAPE);
	else if (i >= FIRST_FACE_SHAPE && i < FIRST_GUMP_SHAPE)
		r = loadRecordBlock(BuildPath(StaticPath, FacesFileName, 0), i, FIRST_FACE_SHAPE);
	else if (i >= FIRST_GUMP_SHAPE && i < FIRST_FILE_BLOCK)
		r = loadRecordBlock(BuildPath(StaticPath, GumpsFileName, 0), i, FIRST_GUMP_SHAPE);
	SpeechPlayer.continuePlaying();
	return r;
}

/* Writes a block out to the file name. */
void U7ShapeManager::writeBlockFile(char *name, int16_t h, int32_t data, int32_t size)
{
	uint8_t ok = 0;
	int16_t handle;

	handle = DosCreate(name);
	if (handle >= 0) {
		WriteHandleFromVoodoo(handle, INT32_C(0), size, data);
		ok = 1;
		DosClose(handle);
	}
	if (!ok)
		ReportErrorSubtype(0xe406, h);
}

/* The pool drops block h: a block still in use is saved to a file first. */
void U7ShapeManager::releaseSlot(int16_t h, CacheEntry *p, uint8_t keep)
{
	int16_t k;

	if (h >= FIRST_BLOCK && h < SHAPE_HANDLES) {
		if (!keep) {
			k = h - FIRST_BLOCK;
			if (!IsDiscardable(k))
				writeBlockFile(BuildNumberedTempPath(CurrentDirectory, TempFileFormat, k, 1), h, p->data,
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
	int16_t i;

	for (i = 0; i < 8; i++)
		names[i] = 0;
	ClearBlockTable(BlockSlotKeys);
	unusedFlag = 0;
}

/* The width and height of a shape's largest frame. */
void U7ShapeManager::getShapeSize(int16_t *w, int16_t *h, int16_t shape)
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
void U7ShapeManager::getFrameSize(int16_t *w, int16_t *h, int16_t shape, int16_t f)
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

void U7ShapeManager::saveUnderShape(View *v, int16_t h, int16_t x, int16_t y, int16_t shape, int16_t f)
{
	SaveUnderFrame(v, get(h), x, y, frame(shape, f), f, 0x101);
}

void U7ShapeManager::saveUnderRect(View *v, int16_t h, Rect *r)
{
	SaveRect(v, get(h), r, 1);
}

void U7ShapeManager::restoreUnderShape(View *v, int16_t h, int16_t x, int16_t y, int16_t shape, int16_t f)
{
	RestoreUnderFrame(v, get(h), x, y, frame(shape, f), f, 0x101);
}

void U7ShapeManager::restoreUnderRect(View *v, int16_t h, Rect *r)
{
	RestoreRect(v, get(h), r, 1);
}

/* Draws a shape; one with more than one frame uses its second when lit. */
void U7ShapeManager::drawShapeSavingUnder(View *v, int16_t h, int16_t x, int16_t y, int16_t shape, int8_t lit)
{
	int16_t f = ShapeManager_getFrameCount(this, shape) == 1 ? 0 : 1;

	saveUnderShape(v, h, x, y, shape, f);
	ShapeManager_draw(this, v, x, y, shape, lit ? f : 0, 0, 0);
}

uint8_t U7ShapeManager::isCursorInBounds(int16_t shape, int16_t f, const Point &a, const Point &b)
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

/* Rewrites the header of a shape from record n of the file. */
void RemapShapeRecord(int16_t shape, int16_t n)
{
	FlexEntry s;
	uint8_t was = gShapeManager.loaded(shape);

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
void CopyShapeHeader(int16_t to, int16_t from)
{
	FlexEntry s;

	ShapesFile.getEntry(from, &s);
	ShapesFile.writeEntry(to, &s);
}

int16_t ClampShapeFrame(int16_t shape, int16_t frame)
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
	int16_t i;

	FrameFlagTable.load(GamedatPath);
	for (i = 0; i < 1024; i++) {
		if (PeekLong(OFFSET(i)) == -INT32_C(1) || PeekLong(OFFSET(i)) == INT32_C(1))
			continue;
		gShapeManager.use(i);
	}
}
