#ifndef SPRITE_H
#define SPRITE_H

#include "objref.h"
#include "text.h"

struct Coord;
struct CellCoord;
struct Rect;
struct Sprite;

/* The table of on-screen sprites: barks, effects and projectiles. */
struct SpriteManager {
	Sprite *sprites;
	uint8_t capacity;
	SpriteManager() { sprites = 0; }
};
extern SpriteManager gSpriteManager;
extern uint8_t ItemSpritesChanged;

void SpriteManager_init(SpriteManager *pool, int16_t capacity);
int16_t SpriteManager_findFreeSprite(SpriteManager *pool);
int16_t SpriteManager_findSpriteForItem(SpriteManager *pool, int16_t item, uint8_t mode);
uint8_t SpriteManager_clearOverlaps(SpriteManager *pool, int16_t shape, Rect *bounds);
int16_t SpriteManager_barkOnItem(SpriteManager *pool, int16_t item, char *text, uint8_t mode, int16_t lifetime,
	uint8_t force);
int16_t SpriteManager_barkAtCoords(SpriteManager *pool, Coord x, Coord y, char *text, int16_t lifetime);
int16_t SpriteManager_playEdgeSprite(SpriteManager *pool, uint8_t direction, int16_t speed, int16_t shape,
	uint8_t frame, int16_t lifetime, uint8_t mode);
int16_t SpriteManager_playSprite(SpriteManager *pool, CellCoord x, CellCoord y, int16_t dx, int16_t dy, int16_t shape,
	uint8_t frame, int16_t lifetime, uint8_t mode);
int16_t SpriteManager_playSpriteForItem(SpriteManager *pool, int16_t item, int16_t x, int16_t y, int16_t dx, int16_t dy, int16_t shape,
	uint8_t frame, int16_t lifetime, uint8_t mode);
void SpriteManager_stopItemSprites(SpriteManager *pool, int16_t item, uint8_t textOnly);
void SpriteManager_stopSprite(SpriteManager *pool, int16_t index);
void UpdateSprites(int8_t advance, Sprite *sprites, int16_t count);

void ClearAllBarks();
void BarkAfterSaving();
void Sprite_getScreenPos(Sprite *, int16_t *, int16_t *);
int8_t DoRectsOverlap(struct Rect *a, struct Rect *b);
void Sprite_clear(Sprite *s);
void Sprite_getBounds(Sprite *s, Rect *bounds);
int16_t Sprite_renderText(Sprite *s, char *text);
int16_t Sprite_initBark(Sprite *s, int16_t item, char *text, uint8_t mode, int16_t lifetime);
int16_t Sprite_initItemEffect(Sprite *s, int16_t item, int16_t x, int16_t y, int16_t dx, int16_t dy, int16_t shape, uint8_t frame,
	uint8_t mode, int16_t lifetime);
void Sprite_initBarkAt(Sprite *s, Coord x, Coord y, char *text, int16_t lifetime);
int16_t Sprite_initEffect(Sprite *s, Coord x, Coord y, int16_t shape, uint8_t frame, int16_t lifetime);
int16_t Sprite_initProjectile(Sprite *s, Coord x, Coord y, int16_t dx, int16_t dy, int16_t shape, uint8_t frame);
int16_t Sprite_initMovingEffect(Sprite *s, Coord x, Coord y, int16_t dx, int16_t dy, int16_t shape, uint8_t frame,
	int16_t lifetime, uint8_t mode);
void Sprite_free(Sprite *s);
int16_t Sprite_getScreenX(Sprite *s);
int16_t Sprite_getScreenY(Sprite *s);
void Sprite_move(Sprite *s);
void Sprite_update(Sprite *s, int8_t advance);
extern uint8_t SpellCastCounts[72];
extern uint8_t SpriteAdvanceToggle;

/* An NPC says a line of the game text above its head. */
inline void BarkLine(objref *who, int16_t line, int8_t mode = 0)
{
	SpriteManager_barkOnItem(&gSpriteManager, *who, GetGameText(1, line), mode, 15, 0);
}

#endif
