/* Black Gate U7.EXE, overlay segment 261 (file offsets 0x07b520 to 0x07be1a, 2298 bytes).
 * Borland C++ 2.0 -mm -O -G -P -Y rebuilds it byte for byte as C++.
 * Original folder unknown.
 */

#include "u7port.h"
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
	int16_t x0, y0;
	Point() { x0 = 0; y0 = 0; }
};

struct Rect : Point {
	int16_t x1, y1;
	Rect() : Point() { x1 = 0; y1 = 0; }
};

/* One bark, effect or projectile: drawn at a world spot, or offset from an item. */
struct Sprite {
	int16_t shape;
	uint8_t mode;
	int16_t lifetime;
	int16_t textHeight;
	int16_t xOffset, yOffset;
	int16_t item;
	uint8_t frame;
	Coord x, y;
	int16_t dx, dy;
	uint8_t flags;
};

struct View { int16_t id; int32_t rowTable; Rect bounds; };

extern int16_t MakeShapeFromImage(int16_t);
extern void ShapeManager_drawInViewport(ShapeManager *, int16_t, int16_t, int16_t, int16_t, int8_t, int16_t);
extern int16_t ShapeManager_getFrameCount(ShapeManager *shapes, int16_t shape);

#define IS_VALID(n) ((int8_t)((n) != 0))
#define SIGN(v) ((v) > 0 ? 1 : (v) < 0 ? -1 : 0)

int8_t DoRectsOverlap(struct Rect *a, struct Rect *b)
{
	int16_t d0, d1, d2, d3;
	int16_t e0, e1, e2, e3;

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

void Sprite_clear(Sprite *s)
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

void Sprite_getBounds(Sprite *s, Rect *bounds)
{
	int16_t x, y, width, height;
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
int16_t Sprite_renderText(Sprite *s, char *text)
{
	char *p;
	int16_t width, image;
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

int16_t Sprite_initBark(Sprite *s, int16_t item, char *text, uint8_t mode, int16_t lifetime)
{
	Sprite_clear(s);
	s->item = item;
	s->mode = mode;
	s->lifetime = lifetime;
	ItemSpritesChanged++;
	return Sprite_renderText(s, text);
}

int16_t Sprite_initItemEffect(Sprite *s, int16_t item, int16_t x, int16_t y, int16_t dx, int16_t dy,
	int16_t shape, uint8_t frame, uint8_t mode, int16_t lifetime)
{
	Sprite_clear(s);
	s->item = item;
	s->shape = shape;
	s->frame = frame;
	s->mode = mode;
	s->lifetime = lifetime;
	Rect bounds;
	Sprite_getBounds(s, &bounds);
	s->xOffset = x - (bounds.x1 - bounds.x0 + 1) / 2;
	s->yOffset = y - (bounds.y1 - bounds.y0 + 1) / 2;
	s->dx = dx;
	s->dy = dy;
	ItemSpritesChanged++;
	return s->shape;
}

void Sprite_initBarkAt(Sprite *s, Coord x, Coord y, char *text, int16_t lifetime)
{
	Sprite_clear(s);
	s->lifetime = lifetime;
	s->x = x;
	s->y = y;
	Sprite_renderText(s, text);
}

int16_t Sprite_initEffect(Sprite *s, Coord x, Coord y, int16_t shape, uint8_t frame, int16_t lifetime)
{
	Sprite_clear(s);
	s->x = x;
	s->y = y;
	s->shape = shape;
	s->frame = frame;
	s->lifetime = lifetime;
	return s->shape;
}

int16_t Sprite_initProjectile(Sprite *s, Coord x, Coord y, int16_t dx, int16_t dy, int16_t shape, uint8_t frame)
{
	Sprite_initEffect(s, x, y, shape, frame, -2);
	s->dx = dx;
	s->dy = dy;
	return s->shape;
}

int16_t Sprite_initMovingEffect(Sprite *s, Coord x, Coord y, int16_t dx, int16_t dy, int16_t shape,
	uint8_t frame, int16_t lifetime, uint8_t mode)
{
	Sprite_initEffect(s, x, y, shape, frame, lifetime);
	s->dx = dx;
	s->dy = dy;
	s->mode = mode;
	return s->shape;
}

void Sprite_free(Sprite *s)
{
	if ((int8_t)(s->shape != -1)) {
		/* shapes built from text or images live in these blocks */
		if ((s->shape >= FIRST_FILE_BLOCK && s->shape < FIRST_BLOCK) ||
			(s->shape >= FIRST_BLOCK && s->shape < SHAPE_HANDLES)) {
			gShapeManager.releaseBlock(s->shape);
		}
		if (IS_VALID(s->item)) {
			ItemSpritesChanged++;
		}
		s->shape = -1;
	}
}

int16_t Sprite_getScreenX(Sprite *s)
{
	int16_t x;
	if (IS_VALID(s->item))
		x = Item_getX(objref(s->item));
	else
		x = s->x;
	x = (x - (int16_t)gCamera.getCenterX()) * 8 + s->xOffset + 160;
	if (IS_VALID(s->item))
		x -= Item_getZ(&objref(s->item)) * 4;
	return x;
}

int16_t Sprite_getScreenY(Sprite *s)
{
	int16_t y;
	if (IS_VALID(s->item))
		y = Item_getY(objref(s->item));
	else
		y = s->y;
	y = (y - (int16_t)gCamera.getCenterY()) * 8 + s->yOffset + 100;
	if (IS_VALID(s->item))
		y -= Item_getZ(&objref(s->item)) * 4;
	return y;
}

void Sprite_getScreenPos(Sprite *s, int16_t *x, int16_t *y)
{
	int16_t width, height;
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

void Sprite_move(Sprite *s)
{
	s->xOffset += s->dx;
	s->yOffset += s->dy;
}

void Sprite_update(Sprite *s, int8_t advance)
{
	int16_t x, y;
	if ((int8_t)(s->shape != -1) == 0) {
		return;
	}
	if (s->item != 0 && (int8_t)(GetItemKind(&objref(s->item)) >= LOCATION_CONTAINED)) {
		Sprite_free(s);
		return;
	}
	Sprite_getScreenPos(s, &x, &y);
	if (advance == 0) {
		return;
	}
	int8_t draw = 1;
	if (s->shape == 1026 && (InDungeon != 0 || CeilingZ < 15)) {
		draw = 0;
	}
	if (draw) {
		ShapeManager_drawInViewport(&gShapeManager, x, y, s->shape, s->frame, s->frame >= 32, 0);
	}
	Sprite_move(s);
	switch (s->lifetime) {
	case -1:    /* runs its frames once */
		s->frame++;
		if (s->frame >= ShapeManager_getFrameCount(&gShapeManager, s->shape)) {
			Sprite_free(s);
		}
		break;
	case -2:    /* a projectile: flies until far off screen */
		if (x < -319 || x > 638 || y < -319 || y > 638) {
			Sprite_free(s);
		}
		break;
	default:
		if (--s->lifetime <= 0) {
			Sprite_free(s);
		}
	}
}

void UpdateSprites(int8_t advance, Sprite *sprites, int16_t count)
{
	if (advance) {
		SpriteAdvanceToggle = 0;
	} else if (!SpriteAdvanceToggle) {
		advance = 1;
		SpriteAdvanceToggle = 1;
	}
	for (; count != 0; count--, sprites++) {
		Sprite_update(sprites, advance);
	}
}

uint8_t SpellCastCounts[72] = { 0 };
SpriteManager gSpriteManager;
uint8_t ItemSpritesChanged = 0;
uint8_t SpriteAdvanceToggle = 0;
