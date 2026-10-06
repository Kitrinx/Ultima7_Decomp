#ifndef SPRITE_H
#define SPRITE_H

#include "objref.h"
#include "text.h"

struct Coord;
struct CellCoord;
struct Rect;
struct Bark;

/* The table of on-screen sprites: barks, effects and projectiles. */
struct SpriteManager {
	Bark *sprites;
	unsigned char capacity;
	SpriteManager() { sprites = 0; }
};
extern SpriteManager gSpriteManager;
extern unsigned char ItemSpritesChanged;

void far SpriteManager_init(SpriteManager *pool, int capacity);
int far SpriteManager_findFreeSprite(SpriteManager *pool);
int far SpriteManager_findSpriteForItem(SpriteManager *pool, int item, unsigned char mode);
unsigned char far SpriteManager_clearOverlaps(SpriteManager *pool, int shape, Rect *bounds);
int far SpriteManager_barkOnItem(SpriteManager *pool, int item, char *text, unsigned char mode, int lifetime,
	unsigned char force);
int far SpriteManager_barkAtCoords(SpriteManager *pool, Coord x, Coord y, char *text, int lifetime);
int far SpriteManager_playEdgeSprite(SpriteManager *pool, unsigned char direction, int speed, int shape,
	unsigned char frame, int lifetime, unsigned char mode);
int far SpriteManager_playSprite(SpriteManager *pool, CellCoord x, CellCoord y, int dx, int dy, int shape,
	unsigned char frame, int lifetime, unsigned char mode);
int far SpriteManager_playSpriteForItem(SpriteManager *pool, int item, int x, int y, int dx, int dy, int shape,
	unsigned char frame, int lifetime, unsigned char mode);
void far SpriteManager_stopItemSprites(SpriteManager *pool, int item, unsigned char textOnly);
void far SpriteManager_stopSprite(SpriteManager *pool, int index);
void far UpdateSprites(char advance, Bark *sprites, int count);

void far ClearAllBarks(int item);
void far BarkAfterSaving();
void far Sprite_getScreenPos(Bark *, int *, int *);
char far DoRectsOverlap(struct Rect *a, struct Rect *b);
void far Sprite_clear(Bark *s);
void far Sprite_getBounds(Bark *s, Rect *bounds);
int far Sprite_renderText(Bark *s, char *text);
int far Sprite_initBark(Bark *s, int item, char *text, unsigned char mode, int lifetime);
int far Sprite_initItemEffect(Bark *s, int item, int x, int y, int dx, int dy, int shape, unsigned char frame,
	unsigned char mode, int lifetime);
void far Sprite_initBarkAt(Bark *s, Coord x, Coord y, char *text, int lifetime);
int far Sprite_initEffect(Bark *s, Coord x, Coord y, int shape, unsigned char frame, int lifetime);
int far Sprite_initProjectile(Bark *s, Coord x, Coord y, int dx, int dy, int shape, unsigned char frame);
int far Sprite_initMovingEffect(Bark *s, Coord x, Coord y, int dx, int dy, int shape, unsigned char frame,
	int lifetime, unsigned char mode);
void far Sprite_free(Bark *s);
int far Sprite_getScreenX(Bark *s);
int far Sprite_getScreenY(Bark *s);
void far Sprite_move(Bark *s);
void far Sprite_update(Bark *s, char advance);
extern unsigned char SpellCastCounts[72];
extern unsigned char SpriteAdvanceToggle;

/* An NPC says a line of the game text above its head. */
inline void BarkLine(objref *who, int line, char mode = 0)
{
	SpriteManager_barkOnItem(&gSpriteManager, *who, GetGameText(1, line), mode, 15, 0);
}

#endif
