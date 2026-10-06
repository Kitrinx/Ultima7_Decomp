/* Serpent Isle SI.EXE, overlay segment 243 (file offsets 0x069d20 to 0x069e94, 372 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
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

extern void far ShapeManager_drawInViewport(ShapeManager *, int, int, int, int, char, int);
extern int far ShapeManager_getFrameCount(ShapeManager *shapes, int shape);

void far Sprite_update(Bark *s, char advance)
{
	int x, y;
	if ((char)(s->shape != -1) == 0) {
		return;
	}
	if (s->item != 0 && (char)(GetItemKind(&objref(s->item)) >= LOCATION_CONTAINED)) {
		Sprite_free(s);
		return;
	}
	Sprite_getScreenPos(s, &x, &y);
	if (advance == 0) {
		return;
	}
	char draw = 1;
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
	case -3:    /* runs its frames backwards once */
		if (s->frame > 0)
			s->frame--;
		else
			Sprite_free(s);
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

void far UpdateSprites(char advance, Bark *sprites, int count)
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
