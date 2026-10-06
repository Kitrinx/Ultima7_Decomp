/* Serpent Isle SI.EXE, overlay segment 221 (file offsets 0x05d8e0 to 0x05e064, 1924 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

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

#define MK_FP(seg, off) ((void _seg *)(seg) + (void near *)(off))
#define ITEM(ref) ((ItemRecord far *)MK_FP(ItemBufferSegment, (ref).off))
#define IS_NPC(ref) \
	((unsigned char)((ItemTypeClassFlags[gItemTypeInfo[ITEM(ref)->typeFrame & 0x3ff].typeClass] & CLASS_NPC) != 0))
#define NPC_FLAG(ref, flag) ((unsigned char)((GetNpcBufferForIbo(&NPCRef(ref))->status & (flag)) != 0))

struct WeaponRef {
	int index;
	WeaponRef(int n) { index = n; }
	WeaponRecord *operator->() { return WeaponRecords.get(index); }
	unsigned char passesBlockers() { return WeaponRecords.get(index)->passesBlockers; }
};

struct AmmoRef {
	int index;
	AmmoRef(int n) { index = n; }
	AmmoRecord *operator->() { return AmmoRecords.get(index); }
	unsigned char passesBlockers() { return AmmoRecords.get(index)->passesBlockers; }
	unsigned char removeOnStop() { return AmmoRecords.get(index)->removeOnStop; }
	unsigned char keepOnStop() { return AmmoRecords.get(index)->keepOnStop; }
};

/* each explosive type has an effect and a sound; each effect has a radius */
unsigned char ExplosionRadii[62] = { 3, 3, 3, 3, 7, 7, 3, 14, 4, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 5, 3, 3, 0, 6, 2 };
int ExplosiveTypes[] = { 554, 856, 32, 621, 704, 639, 399, 287, -1 };
unsigned char ExplosionEffects[] = { 19, 5, 4, 4, 4, 8, 13, 23, 5 };
char ExplosionSounds[] = { 40, 65, 41, 42, 42, -128, 65, 20, -1 };

extern unsigned char far Item_getQuality(objref *ref);
extern void far Item_setQuality(objref *, char);

inline int PlaySprite(SpriteManager *manager, CellCoord x, CellCoord y, int dx, int dy, int shape, char frame,
	int lifetime, char mode)
{
	return SpriteManager_playSprite(manager, x, y, dx, dy, shape, frame, lifetime, mode);
}

inline int PlaySprite(SpriteManager *manager, int item, int x, int y, int dx, int dy, int shape, char frame,
	int lifetime, char mode)
{
	return SpriteManager_playSpriteForItem(manager, item, x, y, dx, dy, shape, frame, lifetime, mode);
}

inline char CheckLineOfFire(ItemId item, CellCoord x, CellCoord y, int width)
{
	return HasLineOfFireToCoords(item, x, y, width);
}

inline char CheckLineOfFire(ItemId item, ItemId target)
{
	return HasLineOfFire(item, target);
}

void EmptyExplodeStub() {}

void Explode(objref attacker, Loc x, Loc y, int width, int weaponNum, int ammoNum, objref projectile)
{
	WeaponRef weapon = weaponNum;
	AmmoRef ammo = ammoNum;
	int radius, effect;
	char sound;
	int index;
	objref target;
	unsigned char area;
	AreaSearch found;

	area = weapon.passesBlockers();
	if (ammoNum != 0) {
		if (ammo.passesBlockers()) area = 1;
	}
	int type = weapon->type;
	if (ammoNum != 0) type = ammo->type;
	char explosive = 0;
	if (ammoNum != 0) {
		if (ammo.removeOnStop() && ammo.keepOnStop()) explosive = 1;
	}
	for (index = 0; ExplosiveTypes[index] != -1; index++) {
		if (ExplosiveTypes[index] == type) break;
	}
	effect = ExplosionEffects[index];
	radius = ExplosionRadii[effect];
	sound = ExplosionSounds[index];
	if (sound != (char)-1) {
		PlaySoundAt(sound, x, y);
	}

	if (explosive) {
		if (type == 639) {  /* death vortex */
			if (projectile.valid()) {
				int frame = Item_getQuality(&projectile);
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
				!(explosive && (unsigned char)(objref(AvatarRef.off).off == attacker.off) && IS_NPC(found.current) &&
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
