/* Black Gate U7.EXE, overlay segment 265 (file offsets 0x07cac0 to 0x07db58, 4248 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Original folder unknown.
 */

#include "objref.h"
#include "lowlevel.h"
#include "iteminfo.h"
#include "coord.h"
#include "collide.h"
#include "script.h"
#include "gtimer.h"
#include "makemojo.h"
#include "misstrac.h"
#include "item.h"
#include "type.h"
#include "eggspawn.h"
#include "search.h"
#include "voolook.h"
#include "trigger.h"
#include "actqueue.h"
#include "weapons.h"
#include "ammo.h"

#define EGG(r) ((EggRecord far *)MK_FP(ItemBufferSegment, ITEM((r).off)->data.extra))
#define TYPE_CLASS(r) (gItemTypeInfo[ITEM((r).off)->typeFrame & 0x3ff].typeClass)
#define IS_EGG(r) ((unsigned char)(TYPE_CLASS(r) == TYPE_CLASS_EGG))
#define IS_NPC(r) ((unsigned char)((ItemTypeClassFlags[TYPE_CLASS(r)] & CLASS_NPC) != 0))

struct Script {
	unsigned char length, data[127];
	Script() { length = 1; }
	unsigned char valid() { return length != 0; }
};

extern int far IsActionQueueRoom();

inline EggRecord far *GetEggData(objref ref) { return EGG(ref); }

inline int GetAmmoType(unsigned type)
{
	CheckMojoBounds(1024L, (unsigned long)type);
	return PeekWord(AmmoLookup.addr + type * 2);
}

struct WeaponRef {
	int index;
	WeaponRef() {}
	WeaponRef(int n) { index = n; }
	WeaponRecord *operator->() { return WeaponRecords.get(index); }
};

struct AmmoRef {
	int index;
	AmmoRef() {}
	AmmoRef(int n) { index = n; }
	AmmoRecord *operator->() { return AmmoRecords.get(index); }
};

extern objref AvatarRef;

inline unsigned char IsUnderRoof(objref r) { return IsUnderMountain(r); }

/* Runs the eggs that act at once: monsters (only when the egg is under a roof exactly when the
 * avatar is), missiles and buttons. Returns whether the egg hatches only once. */
unsigned char far Egg_hatch(objref ref)
{
	if (!EGG(ref)->hatched) {
		Coord x = Item_getX(ref);
		Coord y = Item_getY(ref);
		unsigned char roof;
		switch ((unsigned char)EGG(ref)->kind) {
		case EGG_MONSTER:
			roof = IsUnderRoof(ref);
			if (roof == InDungeon)
				SpawnFromEgg(EGG(ref), ref, x, y);
			break;
		case EGG_MISSILE: Egg_fireMissile(EGG(ref), ref, x, y); break;
		case EGG_BUTTON: Egg_pressButton(EGG(ref), ref, x, y); break;
		}
		if (!EGG(ref)->repeat)
			Egg_setHatched(&ref);
		else
			Egg_clearHatched(&ref);
	}
	return EGG(ref)->once;
}

/* Adds the script step that waits for something to come within the egg's distance. */
void far Egg_addWaitNear(objref *ref, Script *code)
{
	switch ((unsigned char)EGG(*ref)->criteria) {
	case 1: AppendScriptByte(&code->length, 43); break;
	case 2: AppendScriptByte(&code->length, 43); break;
	case 3: AppendScriptByte(&code->length, 30); break;
	default: return;
	}
	AppendScriptByte(&code->length, EGG(*ref)->distance);
}

/* Adds the script step that waits for it to leave again. */
void far Egg_addWaitAway(objref *ref, Script *code)
{
	switch ((unsigned char)EGG(*ref)->criteria) {
	case 1: AppendScriptByte(&code->length, 36); break;
	case 2: AppendScriptByte(&code->length, 36); break;
	case 3: AppendScriptByte(&code->length, 31); break;
	default: return;
	}
	AppendScriptByte(&code->length, EGG(*ref)->distance);
}

/* Queues the egg's script: wait until something comes near (unless forced), do the egg's work,
 * then for a repeating egg wait for it to leave and start over. */
extern "C" void far Egg_activate(objref ref, unsigned char force)
{
	Item_clearTemporary(&ref);
	if (EGG(ref)->nocturnal && !(unsigned char)(GameTime.getHour() < 5 || GameTime.getHour() > 20))
		return;
	int criteria = (unsigned char)EGG(ref)->criteria;
	unsigned char proximity = criteria == 1 || criteria == 2 || criteria == 3;
	if (!force && criteria != 0 && !proximity)
		return;
	Script code;
	if (!force)
		Egg_addWaitNear(&ref, &code);
	AppendScriptByte(&code.length, 72);
	switch ((unsigned char)EGG(ref)->kind) {
	case EGG_MONSTER: case EGG_BUTTON: break;
	case EGG_MISSILE: Egg_addMissileDelay(EGG(ref), &code); break;
	case EGG_MUSIC: Egg_addMusic(EGG(ref), &code); break;
	case EGG_SFX: Egg_addSoundEffect(EGG(ref), &code); break;
	case EGG_SPEECH: Egg_addSpeech(EGG(ref), &code); break;
	case EGG_USECODE: Egg_addUsecode(EGG(ref), &code); break;
	case EGG_TELEPORT: Egg_addTeleport(EGG(ref), &code); break;
	case EGG_WEATHER: Egg_addWeather(EGG(ref), &code); break;
	default: code.length = 0;
	}
	if (proximity != 0 && (unsigned char)EGG(ref)->kind == EGG_MUSIC)
		EGG(ref)->repeat = 1;
	if (EGG(ref)->repeat) {
		if ((unsigned char)EGG(ref)->kind != EGG_MISSILE)
			Egg_addWaitAway(&ref, &code);
		if ((unsigned char)EGG(ref)->kind == EGG_MUSIC) {
			AppendScriptByte(&code.length, SCRIPT_MUSIC);
			AppendScriptByte(&code.length, 0);
			AppendScriptByte(&code.length, 0);
		}
		if (force != 0)
			Egg_addWaitNear(&ref, &code);
		if (proximity != 0)
			AppendScriptByte(&code.length, 10);
	}
	if (code.valid())
		ActionQueue.add(ref.off, (char *)&code);
}

void far Egg_addMusic(EggRecord far *egg, Script *code)
{
	AppendScriptByte(&code->length, SCRIPT_MUSIC);
	AppendScriptByte(&code->length, egg->data1.bytes.first);
	AppendScriptByte(&code->length, egg->data1.bytes.second);
}

void far Egg_addSoundEffect(EggRecord far *egg, Script *code)
{
	AppendScriptByte(&code->length, SCRIPT_SFX);
	AppendScriptByte(&code->length, egg->data1.bytes.first);
}

void far Egg_addSpeech(EggRecord far *egg, Script *code)
{
	AppendScriptByte(&code->length, SCRIPT_SPEECH);
	AppendScriptByte(&code->length, egg->data1.bytes.first);
}

void far Egg_addUsecode(EggRecord far *egg, Script *code)
{
	AppendScriptByte(&code->length, SCRIPT_USECODE);
	AppendScriptWord(&code->length, egg->data2.word);
}

/* Fires the egg's weapon (data1 its type) at the avatar (direction 8) or in direction data2. */
void far Egg_fireMissile(EggRecord far *egg, objref ref, Coord x, Coord y)
{
	WeaponRef weapon;
	AmmoRef ammo;
	objref projectile;
	TypeFrame typeFrame;
	unsigned char attack;
	weapon.index = WeaponLookup.get(egg->data1.word & 0x3ff);
	ammo.index = GetAmmoType(egg->data1.word & 0x3ff);
	if (ammo->projectile != ammo->family && ammo->projectile != -1) {
		if (ammo->projectile == -3)
			typeFrame = egg->data1.word;
		else
			typeFrame = ammo->projectile;
	} else {
		if (weapon->projectile == -3)
			typeFrame = egg->data1.word;
		else
			typeFrame = weapon->projectile;
	}
	if ((unsigned char)weapon->missileSpeed)
		attack = 3;
	else if (!weapon->speed)
		attack = 2;
	else if (weapon->speed > 2)
		attack = 0;
	else
		attack = 1;
	if (!CreateItem(&projectile, typeFrame.bits, x, y, Item_getZ(&ref)))
		return;
	if (egg->data2.bytes.first == 8) {
		if (!FireMissileAtItem(projectile, weapon.index, ammo.index, ref, 60, AvatarRef, attack))
			return;
	} else {
		if (!FireMissileInDirection(projectile, weapon.index, ammo.index, ref, 25, egg->data2.bytes.first,
			attack))
			return;
	}
	if ((unsigned char)IsActionQueueRoom())
		ActionQueue.add(ref.off, (char *)MakeScript(SCRIPT_NO_HALT, SCRIPT_SFX, 7, SCRIPT_END));
}

void far Egg_addMissileDelay(EggRecord far *egg, Script *code)
{
	if (egg->data2.bytes.second != 0) {
		AppendScriptByte(&code->length, SCRIPT_WAIT);
		AppendScriptByte(&code->length, egg->data2.bytes.second);
		egg->repeat = 1;
	}
}

void far Egg_addTeleport(EggRecord far *egg, Script *code)
{
	AppendScriptByte(&code->length, SCRIPT_TELEPORT);
	AppendScriptByte(&code->length, egg->data1.bytes.first);
}

/* Sets weather data1.first for data1.second (a long wait when 0), then clears it. */
void far Egg_addWeather(EggRecord far *egg, Script *code)
{
	AppendScriptByte(&code->length, SCRIPT_FINISH);
	AppendScriptByte(&code->length, SCRIPT_WEATHER);
	AppendScriptByte(&code->length, egg->data1.bytes.first);
	if (egg->data1.bytes.second == 0) {
		AppendScriptByte(&code->length, 41);
		AppendScriptByte(&code->length, 100);
	} else {
		AppendScriptByte(&code->length, 40);
		AppendScriptByte(&code->length, egg->data1.bytes.second);
	}
	AppendScriptByte(&code->length, SCRIPT_WEATHER);
	AppendScriptByte(&code->length, 255);
}

/* Activates every button-hatched egg within the egg's distance (10 when 0). */
void far Egg_pressButton(EggRecord far *egg, objref ref, Coord x, Coord y)
{
	AreaSearch found;
	if (egg->data1.bytes.first == 0)
		egg->data1.bytes.first = 10;
	FindItemInArea(&found, x - egg->data1.bytes.first, y - egg->data1.bytes.first,
		x + egg->data1.bytes.first, y + egg->data1.bytes.first, 0x10, -1, -1, 255);
	while (found.current.valid()) {
		if (IS_EGG(found.current) && (unsigned char)GetEggData(found.current)->criteria == CRITERIA_BUTTON)
			Egg_activate(found.current, 1);
		FindItem(&found);
	}
}

void far ActivateEgg(int ref)
{
	Egg_activate(ref, 1);
}

/* Activates the stepped-on eggs whose distance reaches x, y; NPCs trigger none. */
unsigned char far TriggerEggsUnderItem(objref ref, CellCoord x, CellCoord y)
{
	unsigned char triggered;
	if (IS_NPC(ref))
		return 0;
	AreaSearch found;
	triggered = 0;
	FindItemInArea(&found, x - 16, y - 16, x + 16, y + 16, 0x10, -1, -1, 255);
	while (found.current.valid()) {
		if (IS_EGG(found.current) && !GetEggData(found.current)->hatched &&
			(unsigned char)GetEggData(found.current)->criteria == CRITERIA_STEPPED_ON &&
			GetDistance(x, y, Item_getX(found.current), Item_getY(found.current)) <=
				(int)GetEggData(found.current)->distance) {
			Egg_activate(found.current, 1);
			triggered = 1;
		}
		FindItem(&found);
	}
	return triggered;
}
