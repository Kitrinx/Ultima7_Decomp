/* Serpent Isle SI.EXE, overlay segment 247 (file offsets 0x06b2e0 to 0x06b852, 1394 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include "objref.h"
#include "iteminfo.h"
#include "coord.h"
#include "collide.h"
#include "script.h"
#include "gtimer.h"
#include "item.h"
#include "type.h"
#include "eggspawn.h"
#include "voolook.h"
#include "trigger.h"
#include "actqueue.h"

#define EGG(r) ((EggRecord *)ItemAt(ITEM((r).off)->data.extra))

struct Script {
	uint8_t length, data[127];
	Script() { length = 1; }
	uint8_t valid() { return length != 0; }
};

extern objref AvatarRef;

inline uint8_t IsUnderRoof(objref r) { return IsUnderMountain(r); }

/* Runs the eggs that act at once, when the avatar is within four levels of the egg: monsters (only
 * when the egg and the avatar are both or neither under a roof), missiles and buttons. Returns
 * whether the egg hatches only once. */
uint8_t Egg_hatch(objref ref)
{
	if (!EGG(ref)->hatched) {
		int16_t eggZ = Item_getZ(&ref);
		int16_t avatarZ = Item_getZ(&AvatarRef);
		int16_t dz = avatarZ - eggZ;
		if (dz < 0)
			dz = -dz;
		if (dz > 4)
			return 0;
		Coord x = Item_getX(ref);
		Coord y = Item_getY(ref);
		uint8_t roof;
		switch ((uint8_t)EGG(ref)->kind) {
		case EGG_MONSTER:
			roof = IsUnderRoof(ref);
			if (!roof && !InDungeon || roof && InDungeon)
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
void Egg_addWaitNear(objref *ref, Script *code)
{
	switch ((uint8_t)EGG(*ref)->criteria) {
	case 1: AppendScriptByte(&code->length, 43); break;
	case 2: AppendScriptByte(&code->length, 43); break;
	case 3: AppendScriptByte(&code->length, 30); break;
	default: return;
	}
	AppendScriptByte(&code->length, EGG(*ref)->distance);
}

/* Adds the script step that waits for it to leave again. */
void Egg_addWaitAway(objref *ref, Script *code)
{
	switch ((uint8_t)EGG(*ref)->criteria) {
	case 1: AppendScriptByte(&code->length, 36); break;
	case 2: AppendScriptByte(&code->length, 36); break;
	case 3: AppendScriptByte(&code->length, 31); break;
	default: return;
	}
	AppendScriptByte(&code->length, EGG(*ref)->distance);
}

/* Queues the egg's script: wait until something comes near (unless forced), do the egg's work,
 * then for a repeating egg wait for it to leave and start over. */
extern "C" void Egg_activate(objref ref, uint8_t force)
{
	Item_clearTemporary(&ref);
	if (EGG(ref)->nocturnal && !(uint8_t)(GameTime.getHour() < 5 || GameTime.getHour() > 20))
		return;
	int16_t criteria = (uint8_t)EGG(ref)->criteria;
	uint8_t proximity = criteria == 1 || criteria == 2 || criteria == 3;
	if (!force && criteria != 0 && !proximity)
		return;
	Script code;
	if (!force)
		Egg_addWaitNear(&ref, &code);
	AppendScriptByte(&code.length, 72);
	switch ((uint8_t)EGG(ref)->kind) {
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
	if (proximity != 0 && (uint8_t)EGG(ref)->kind == EGG_MUSIC)
		EGG(ref)->repeat = 1;
	if (EGG(ref)->repeat) {
		if ((uint8_t)EGG(ref)->kind != EGG_MISSILE)
			Egg_addWaitAway(&ref, &code);
		if ((uint8_t)EGG(ref)->kind == EGG_MUSIC) {
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

