/* Serpent Isle SI.EXE, overlay segment 242 (file offsets 0x069570 to 0x069cf0, 1920 bytes).
 * Borland C++ 2.0 -mm -O -G -P -Y rebuilds it byte for byte as C++.
 * Filename and folder inferred.
 */

#include "lowlevel.h"
#include "objref.h"
#include "iteminfo.h"
#include "coord.h"
#include "collide.h"
#include "u7manage.h"
#include "itable.h"
#include "camera.h"
#include "item.h"
#include "mapview.h"
#include "sprite.h"

struct Point {
	int x0, y0;
	Point() { x0 = 0; y0 = 0; }
};

struct Rect : Point {
	int x1, y1;
	Rect() : Point() { x1 = 0; y1 = 0; }
};

/* One bark, effect or projectile: drawn at a world spot, or offset from an item. */
struct Bark {
	int shape;
	unsigned char mode;
	int lifetime;
	int textHeight;
	int xOffset, yOffset;
	int item;
	unsigned char frame;
	Coord x, y;
	int dx, dy;
	unsigned char flags;
};

struct View { int id; long rowTable; Rect bounds; };

extern int far MakeShapeFromImage(int);
extern void far ShapeManager_drawInViewport(ShapeManager *, int, int, int, int, char, int);
extern int far ShapeManager_getFrameCount(ShapeManager *shapes, int shape);

#define IS_VALID(n) ((char)((n) != 0))
#define SIGN(v) ((v) > 0 ? 1 : (v) < 0 ? -1 : 0)

char far DoRectsOverlap(struct Rect *a, struct Rect *b)
{
	int d0, d1, d2, d3;
	int e0, e1, e2, e3;

	d0 = b->x0 - a->x0;
	d1 = b->x1 - a->x0;
	d2 = b->x0 - a->x1;
	d3 = b->x1 - a->x1;
	if (SIGN(d0) == SIGN(d1) && SIGN(d2) == SIGN(d3))
		return 0;
	e0 = b->y0 - a->y0;
	e1 = b->y1 - a->y0;
	e2 = b->y0 - a->y1;
	e3 = b->y1 - a->y1;
	if (SIGN(e0) != SIGN(e1) || SIGN(e2) != SIGN(e3))
		return 1;
	return 0;
}

void far Sprite_clear(Bark *s)
{
	s->lifetime = 0;
	s->mode = 0;
	s->textHeight = 0;
	s->xOffset = 0;
	s->yOffset = 0;
	s->item = 0;
	s->frame = 0;
	s->x = 0;
	s->y = 0;
	s->dx = 0;
	s->dy = 0;
	s->flags = 0;
}

void far Sprite_getBounds(Bark *s, Rect *bounds)
{
	int x, y, width, height;
	Sprite_getScreenPos(s, &x, &y);
	gShapeManager.getFrameSize(&width, &height, s->shape, s->frame);
	if (s->frame < 32) {
		bounds->x0 = x;
		bounds->y0 = y;
		bounds->x1 = x + width;
		bounds->y1 = y + height;
	} else {
		bounds->x0 = y;
		bounds->y0 = x;
		bounds->x1 = y + height;
		bounds->y1 = x + width;
	}
}

/* Renders a bark's text into a new shape block; '@' in the text prints as a double quote. */
int far Sprite_renderText(Bark *s, char *text)
{
	char *p;
	int width, image;
	Rect *bounds;
	for (p = text; *p != 0; p++) {
		if (*p == '@') {
			*p = '"';
		}
	}
	s->textHeight = YellowTextPrinter.charHeight(0);
	width = YellowTextPrinter.textWidth(text) + 6;
	image = gShapeManager.allocateView(0, 0, 319, s->textHeight + 2);
	FillView(gShapeManager.lockView(image), -1);
	YellowTextPrinter.target = gShapeManager.lockView(image);
	YellowTextPrinter.x = 0;
	YellowTextPrinter.y = s->textHeight;
	YellowTextPrinter.printString(text);
	bounds = &gShapeManager.lockView(image)->bounds;
	bounds->x0 = 0;
	bounds->x1 = width;
	s->shape = MakeShapeFromImage(image);
	gShapeManager.releaseBlock(image);
	return s->shape;
}

int far Sprite_initBark(Bark *s, int item, char *text, unsigned char mode, int lifetime)
{
	Sprite_clear(s);
	s->item = item;
	s->mode = mode;
	s->lifetime = lifetime;
	ItemSpritesChanged++;
	return Sprite_renderText(s, text);
}

int far Sprite_initItemEffect(Bark *s, int item, int x, int y, int dx, int dy,
	int shape, unsigned char frame, unsigned char mode, int lifetime)
{
	Sprite_clear(s);
	s->item = item;
	s->shape = shape;
	s->frame = frame;
	s->mode = mode;
	s->lifetime = lifetime;
	Rect bounds;
	Sprite_getBounds(s, &bounds);
	s->xOffset = x;
	s->yOffset = y;
	s->dx = dx;
	s->dy = dy;
	ItemSpritesChanged++;
	return s->shape;
}

void far Sprite_initBarkAt(Bark *s, Coord x, Coord y, char *text, int lifetime)
{
	Sprite_clear(s);
	s->lifetime = lifetime;
	s->x = x;
	s->y = y;
	Sprite_renderText(s, text);
}

int far Sprite_initEffect(Bark *s, Coord x, Coord y, int shape, unsigned char frame, int lifetime)
{
	Sprite_clear(s);
	s->x = x;
	s->y = y;
	s->shape = shape;
	s->frame = frame;
	s->lifetime = lifetime;
	return s->shape;
}

int far Sprite_initProjectile(Bark *s, Coord x, Coord y, int dx, int dy, int shape, unsigned char frame)
{
	Sprite_initEffect(s, x, y, shape, frame, -2);
	s->dx = dx;
	s->dy = dy;
	return s->shape;
}

int far Sprite_initMovingEffect(Bark *s, Coord x, Coord y, int dx, int dy, int shape,
	unsigned char frame, int lifetime, unsigned char mode)
{
	Sprite_initEffect(s, x, y, shape, frame, lifetime);
	s->dx = dx;
	s->dy = dy;
	s->mode = mode;
	return s->shape;
}

void far Sprite_free(Bark *s)
{
	if ((char)(s->shape != -1)) {
		/* shapes built from text or images live in these blocks */
		if ((s->shape >= FIRST_FILE_BLOCK && s->shape < FIRST_BLOCK) ||
			(s->shape >= FIRST_BLOCK && s->shape < SHAPE_HANDLES)) {
			gShapeManager.releaseBlock(s->shape);
		}
		if (IS_VALID(s->item)) {
			ItemSpritesChanged--;
		}
		s->shape = -1;
	}
}

int far Sprite_getScreenX(Bark *s)
{
	int x;
	if (IS_VALID(s->item))
		x = Item_getX(objref(s->item));
	else
		x = s->x;
	x = (x - (int)gCamera.getCenterX()) * 8 + s->xOffset + 160;
	if (IS_VALID(s->item))
		x -= Item_getZ(&objref(s->item)) * 4;
	return x;
}

int far Sprite_getScreenY(Bark *s)
{
	int y;
	if (IS_VALID(s->item))
		y = Item_getY(objref(s->item));
	else
		y = s->y;
	y = (y - (int)gCamera.getCenterY()) * 8 + s->yOffset + 100;
	if (IS_VALID(s->item))
		y -= Item_getZ(&objref(s->item)) * 4;
	return y;
}

void far Sprite_getScreenPos(Bark *s, int *x, int *y)
{
	int width, height;
	if (IS_VALID(s->textHeight) && IS_VALID(s->item)) {
		objref owner(s->item);
		gShapeManager.getFrameSize(&width, &height, ITEM(owner.off)->typeFrame & 0x3ff, s->frame);
		width = -width;
		height = -(height + s->textHeight + 2);
		if (GetMagnitude(width - s->xOffset) > 2) {
			s->xOffset = width;
		}
		if (GetMagnitude(height - s->yOffset) > 2) {
			s->yOffset = height;
		}
	}
	*x = Sprite_getScreenX(s);
	*y = Sprite_getScreenY(s);
}

void far Sprite_move(Bark *s)
{
	s->xOffset += s->dx;
	s->yOffset += s->dy;
}

unsigned char SpellCastCounts[72] = { 0 };
SpriteManager gSpriteManager;
unsigned char ItemSpritesChanged = 0;
unsigned char SpriteAdvanceToggle = 0;
