/* Serpent Isle SI.EXE, overlay segment 221 (file offsets 0x05d8e0 to 0x05e064, 1924 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include "lowlevel.h"
#include "itemrec.h"
#include "objref.h"
#include "u7npc.h"
#include "makemojo.h"
#include "sprite.h"
#include "random.h"
#include "combwpn.h"
#include "type.h"
#include "npcref.h"
#include "coord.h"
#include "sounds.h"
#include "search.h"
#include "missile.h"
#include "weapons.h"
#include "ammo.h"

#define ITEM(ref) ((ItemRecord *)ItemAt((ref).off))
#define IS_NPC(ref) \
	((uint8_t)((ItemTypeClassFlags[gItemTypeInfo[ITEM(ref)->typeFrame & 0x3ff].typeClass] & CLASS_NPC) != 0))
#define NPC_FLAG(ref, flag) ((uint8_t)((GetNpcBufferForIbo(&NPCRef(ref))->status & (flag)) != 0))

struct WeaponRef {
	int16_t index;
	WeaponRef(int16_t n) { index = n; }
	WeaponRecord *operator->() { return WeaponRecords.get(index); }
	uint8_t passesBlockers() { return WeaponRecords.get(index)->passesBlockers; }
};

struct AmmoRef {
	int16_t index;
	AmmoRef(int16_t n) { index = n; }
	AmmoRecord *operator->() { return AmmoRecords.get(index); }
	uint8_t passesBlockers() { return AmmoRecords.get(index)->passesBlockers; }
	uint8_t removeOnStop() { return AmmoRecords.get(index)->removeOnStop; }
	uint8_t keepOnStop() { return AmmoRecords.get(index)->keepOnStop; }
};

/* each explosive type has an effect and a sound; each effect has a radius */
extern const uint8_t ExplosionRadii[62] = { 3, 3, 3, 3, 7, 7, 3, 14, 4, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 5, 3, 3, 0, 6, 2 };
extern const int16_t ExplosiveTypes[] = { 554, 856, 32, 621, 704, 639, 399, 287, -1 };
extern const uint8_t ExplosionEffects[] = { 19, 5, 4, 4, 4, 8, 13, 23, 5 };
extern const char ExplosionSounds[] = { 40, 65, 41, 42, 42, -128, 65, 20, -1 };

extern uint8_t Item_getQuality(objref *ref);
extern void Item_setQuality(objref *, int8_t);

inline int16_t PlaySprite(SpriteManager *manager, CellCoord x, CellCoord y, int16_t dx, int16_t dy, int16_t shape, int8_t frame,
	int16_t lifetime, int8_t mode)
{
	return SpriteManager_playSprite(manager, x, y, dx, dy, shape, frame, lifetime, mode);
}

inline int16_t PlaySprite(SpriteManager *manager, int16_t item, int16_t x, int16_t y, int16_t dx, int16_t dy, int16_t shape, int8_t frame,
	int16_t lifetime, int8_t mode)
{
	return SpriteManager_playSpriteForItem(manager, item, x, y, dx, dy, shape, frame, lifetime, mode);
}

inline int8_t CheckLineOfFire(ItemId item, CellCoord x, CellCoord y, int16_t width)
{
	return HasLineOfFireToCoords(item, x, y, width);
}

inline int8_t CheckLineOfFire(ItemId item, ItemId target)
{
	return HasLineOfFire(item, target);
}

void EmptyExplodeStub() {}

void Explode(objref attacker, Loc x, Loc y, int16_t width, int16_t weaponNum, int16_t ammoNum, objref projectile)
{
	WeaponRef weapon = weaponNum;
	AmmoRef ammo = ammoNum;
	int16_t radius, effect;
	int8_t sound;
	int16_t index;
	objref target;
	uint8_t area;
	AreaSearch found;

	area = weapon.passesBlockers();
	if (ammoNum != 0) {
		if (ammo.passesBlockers()) area = 1;
	}
	int16_t type = weapon->type;
	if (ammoNum != 0) type = ammo->type;
	int8_t explosive = 0;
	if (ammoNum != 0) {
		if (ammo.removeOnStop() && ammo.keepOnStop()) explosive = 1;
	}
	for (index = 0; ExplosiveTypes[index] != -1; index++) {
		if (ExplosiveTypes[index] == type) break;
	}
	effect = ExplosionEffects[index];
	radius = ExplosionRadii[effect];
	sound = ExplosionSounds[index];
	if (sound != (int8_t)-1) {
		PlaySoundAt(sound, x, y);
	}

	if (explosive) {
		if (type == 639) {  /* death vortex */
			if (projectile.valid()) {
				int16_t frame = Item_getQuality(&projectile);
				frame = (frame + 1) & 15;
				Item_setQuality(&projectile, frame);
				PlaySprite(&gSpriteManager, x - width / 2, y - width / 2, 0, 0, effect + 1024, frame, 2, 5);
			}
		} else
			PlaySprite(&gSpriteManager, x - width / 2, y - width / 2, 0, 0, effect + 1024,
				GenerateRandomIntegerInRange(8), 2, 5);
	} else {
		PlaySprite(&gSpriteManager, x - width / 2, y - width / 2, 0, 0, effect + 1024, 0, -1, 5);
	}

	if (FindItemInArea(&found, x - radius, y - radius, x + radius, y + radius, 0, -1, -1, 255)) {
		while (found.current.valid()) {
			if (attacker.off != found.current.off &&
				!(IS_NPC(found.current) && NPC_FLAG(found.current, NPC_DEAD)) &&
				!(explosive && (uint8_t)(objref(AvatarRef.off).off == attacker.off) && IS_NPC(found.current) &&
					NPC_FLAG(found.current, NPC_IN_PARTY)) &&
				(area || CheckLineOfFire(found.current, x, y, width)))
				target.off = found.current.off;
			else
				target.off = 0;
			FindItem(&found);
			if (target.valid())
				ApplyWeaponHit(attacker, target, weaponNum, ammoNum, 0, 1);
		}
	}
}
