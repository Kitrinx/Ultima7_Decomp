/* Serpent Isle SI.EXE, overlay segment 358 (file offsets 0x0b30e0 to 0x0b43e4, 4868 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

/* path: misstrac.c */
#include "u7port.h"
#include "itemrec.h"
#include "iteminfo.h"
#include "lowlevel.h"
#include "typefram.h"
#include "type.h"
#include "item.h"
#include "npcref.h"
#include "coord.h"
#include "u7npc.h"
#include "debug.h"
#include "makemojo.h"
#include "missile.h"
#include "sortitem.h"
#include "script.h"
#include "actqueue.h"
#include "combatai.h"
#include "misstrac.h"
#include "weapons.h"
#include "ammo.h"

struct Path {
	uint8_t majorDirection;
	int16_t initialDistance, stepsRemaining, middleError, middleStep, middleCorrection;
	uint8_t middleDirection;
	int16_t minorError, minorStep, minorCorrection;
	uint8_t minorDirection, state;
	int16_t x, y, z;
	Path() { state = 0; }
	void initialize(Coord startX, Coord startY, int16_t startZ, Coord targetX, Coord targetY, int16_t targetZ);
};
struct MissileTracker : Path {
	uint16_t missile;
	uint8_t active, mode;
	int16_t weapon, ammo;
	objref attacker;
	uint8_t damage;
	objref target;
	uint8_t kind;
	int16_t range;
	Coord targetX, targetY;
	uint8_t counted;
	uint32_t storedChecksum;
	void checksum(char *where);
	void setFrame();
	uint8_t update();
	uint8_t stop();
	uint8_t fire(ItemId, Coord, Coord, int16_t, int16_t, uint8_t);
	void updateChecksum() {
		storedChecksum = (uint16_t)(missile + mode + weapon + ammo + attacker.off + damage
			+ target.off + kind + range + targetX.value + targetY.value + counted);
	}
};
extern "C" int16_t GetPartyIndex(objref);
struct WeaponRef {
	int16_t index;
	WeaponRef(int16_t n) { index = n; }
};
struct AmmoRef {
	int16_t index;
	AmmoRef(int16_t n) { index = n; }
};
struct Script { uint8_t length, bytes[127]; Script() { length = 1; } };
inline Coord GetX(objref ref) { return Item_getX(ref); }
inline Coord GetY(objref ref) { return Item_getY(ref); }

inline void MakeTakeable(objref ref) { Item_setOkayToTake(&ref); }

uint8_t MissileTracker::fire(ItemId attacker,
	Coord aimX, Coord aimY, int16_t aimZ, int16_t slot, uint8_t directional)
{
	Coord x, y;
	int16_t z;
	objref actor;
	TypeFrame typeFrame;
	uint8_t stopped, returning, area;
	int16_t reach;

	checksum("fire 0");
	if ((uint8_t)(ItemTypeClassFlags[gItemTypeInfo[objref(missile).type()].typeClass] & CLASS_QUALITY_FLAGS)) {
		active = 0;
		return 0;
	}
	if ((uint8_t)Item_hasHitPoints(&objref(missile))) {
		active = 0;
		return 0;
	}
	counted = 0;
	targetX = GetX(target);
	targetY = GetY(target);
	updateChecksum();
	actor = objref(attacker.off);
	typeFrame.bits = actor.ptr()->typeFrame;
	x = Coord((int16_t)Item_getX(actor) - (GetFootprintX(typeFrame) >> 1));
	y = Coord((int16_t)Item_getY(actor) - (GetFootprintY(typeFrame) >> 1));
	z = (uint8_t)(Item_getZ(&actor) + gItemTypeInfo[actor.type()].height) - 1;
	if (z < 0)
		z = 0;
	if (missile) {
		Item_move(&objref(missile), x, y, z);
	}
	initialize(x, y, z, aimX, aimY, aimZ);
	if (state == 1) {
		active = 0;
		updateChecksum();
		return 1;
	}
	range = 0;
	updateChecksum();
	WeaponRecord record;
	AmmoRecord ammunition;
	if (weapon) {
		int16_t weaponIndex = weapon;
		WeaponRecords.read(weaponIndex, &record);
		returning = record.returns;
		area = record.passesBlockers;
		if (record.homing) {
			counted = 1;
			if (record.range == 31)
				range = 6000;
			else
				range = record.range * 10;
			updateChecksum();
		} else {
			if (ammo) {
				int16_t ammoIndex = ammo;
				AmmoRecords.read(ammoIndex, &ammunition);
				if (ammunition.returns)
					returning = 1;
				if (ammunition.passesBlockers)
					area = 1;
			}
			if (returning) {
				PartyMissileFlags[GetPartyIndex(NPCRef(attacker.off))] = 1;
				counted = 1;
				range = record.range * 10;
				updateChecksum();
			}
			reach = record.range;
			if (record.uses == 2 || record.uses == 1) {
				if (actor.isNpc()) {
					reach = NPCRef(actor).strength();
					if (record.uses == 2)
						reach <<= 1;
				} else
					reach = 31;
			}
			if (reach < initialDistance || directional) {
				initialDistance = reach;
				stepsRemaining = initialDistance << 3;
			} else if (!area && target.valid())
				stepsRemaining = reach << 3;
		}
	}
	setFrame();
	stopped = update();
	if (!stopped && missile && kind != 4) {
		Script command;
		AppendScriptByte(&command.length, 46);
		AppendScriptByte(&command.length, kind == 3 ? 34 : 2);
		AppendScriptByte(&command.length, 121);
		AppendScriptByte(&command.length, slot);
		AppendScriptByte(&command.length, SCRIPT_LOOP);
		AppendScriptByte(&command.length, -3);
		AppendScriptByte(&command.length, 0xff);
		ActionQueue.add(missile, (char *)&command);
	} else {
		if (kind == 4) {
			while (!stopped)
				stopped = update();
			if (state == 0) {
				active = 0;
				updateChecksum();
				return 0;
			}
		}
		if (stop() && missile) {
			actor = missile;
			Item_delete(&actor);
		}
		return 1;
	}
	if (counted)
		++ActiveMissiles;
	return 1;
}

uint8_t MissileTracker::stop()
{
	objref ref;
	uint8_t remove = 0;
	uint8_t retainOnStop, removeOnStop;
	if (active) {
		checksum("stop 0");
		if (missile) {
			if (missile == (uint16_t)DetachedItems)
				remove = 1;
			else if (weapon) {
				ref = objref(missile);
				Item_setFrame(&ref, 0);
				WeaponRecord weaponInfo;
				int16_t weaponIndex = weapon;
				WeaponRecords.read(weaponIndex, &weaponInfo);
				retainOnStop = 0;
				removeOnStop = 0;
				if (weaponInfo.ammo == -3 && ammo)
					retainOnStop = 1;
				else if (weaponInfo.ammo == -1 || weaponInfo.ammo == -2 || weaponInfo.explodes)
					removeOnStop = 1;
				if (ammo) {
					AmmoRecord ammoInfo;
					int16_t ammoIndex = ammo;
					AmmoRecords.read(ammoIndex, &ammoInfo);
					if (ammoInfo.keepOnStop)
						retainOnStop = 1;
					if (ammoInfo.removeOnStop || ammoInfo.explodes)
						removeOnStop = 1;
				}
				if (removeOnStop || (!retainOnStop && state == 1))
					remove = 1;
			}
		}
		active = 0;
		updateChecksum();
		if (counted)
			--ActiveMissiles;
	}
	return remove;
}

int16_t AllocMissileTracker()
{
	int16_t slot = MISSILE_COUNT;
	int16_t i;
	for (i = 0; i < MISSILE_COUNT; i++) {
		if (!MissileTrackers[i].active) {
			MissileTrackers[i].active = 1;
			slot = i;
			break;
		}
	}
	return slot;
}

void ResetMissileTrackers()
{
	int16_t i;
	for (i = 0; i < MISSILE_COUNT; i++)
		MissileTrackers[i].active = 0;
	ActiveMissiles = 0;
}

uint8_t FireMissileAtCoords(ItemId missile, int16_t weapon, int16_t ammo, ItemId attacker,
	uint8_t damage, Coord targetX, Coord targetY, int16_t targetZ, uint8_t kind)
{
	int16_t slot;
	MissileTracker *tracker;
	objref target;

	slot = AllocMissileTracker();
	if (slot != MISSILE_COUNT) {
		tracker = &MissileTrackers[slot];
		if (missile.valid()) {
			MakeTakeable(missile.off);
		}
		target = 0;
		tracker->mode = 0;
		tracker->missile = missile.off;
		tracker->weapon = weapon;
		tracker->ammo = ammo;
		tracker->attacker = attacker.off;
		tracker->damage = damage;
		tracker->target = target;
		tracker->kind = kind;
		tracker->updateChecksum();
		return tracker->fire(attacker, targetX, targetY, targetZ, slot, 0);
	}
	return 0;
}

uint8_t FireMissileAtItem(ItemId missile, int16_t weapon, int16_t ammo, ItemId attacker,
	uint8_t damage, ItemId target, uint8_t kind)
{
	Coord x, y;
	int16_t z, slot;
	objref ref;
	TypeFrame typeFrame;
	MissileTracker *tracker;

	slot = AllocMissileTracker();
	if (slot != MISSILE_COUNT) {
		tracker = &MissileTrackers[slot];
		if (missile.valid()) {
			MakeTakeable(missile.off);
		}
		ref = objref(target.off);
		typeFrame.bits = ref.ptr()->typeFrame;
		x = Item_getX(ref);
		x -= GetFootprintX(typeFrame) >> 1;
		y = Item_getY(ref);
		y -= GetFootprintY(typeFrame) >> 1;
		z = Item_getZ(&ref) + (gItemTypeInfo[typeFrame.bits & 0x3ff].height >> 1);
		if (z > 15)
			z = 15;
		tracker->mode = 0;
		tracker->missile = missile.off;
		tracker->weapon = weapon;
		tracker->ammo = ammo;
		tracker->attacker = attacker.off;
		tracker->damage = damage;
		tracker->target = target.off;
		tracker->kind = kind;
		tracker->updateChecksum();
		return tracker->fire(attacker, x, y, z, slot, 0);
	}
	return 0;
}

uint8_t FireMissileInDirection(ItemId missile, int16_t weapon, int16_t ammo, ItemId attacker,
	uint8_t damage, uint8_t direction, uint8_t kind)
{
	Coord x, y;
	int16_t z;
	objref target, ref;
	TypeFrame typeFrame;
	int16_t slot;
	MissileTracker *tracker;

	slot = AllocMissileTracker();
	if (slot != MISSILE_COUNT) {
		tracker = &MissileTrackers[slot];
		if (missile.valid()) {
			MakeTakeable(missile.off);
		}
		target = 0;
		ref = attacker.off;
		typeFrame.bits = ref.ptr()->typeFrame;
		x = Coord(Coord((int16_t)Item_getX(ref) - (GetFootprintX(typeFrame) >> 1)) + DirDeltaX[direction]);
		y = Coord(Coord((int16_t)Item_getY(ref) - (GetFootprintY(typeFrame) >> 1)) + DirDeltaY[direction]);
		z = (uint8_t)(Item_getZ(&ref) + gItemTypeInfo[ref.type()].height) - 1;
		tracker->mode = 0;
		tracker->missile = missile.off;
		tracker->weapon = weapon;
		tracker->ammo = ammo;
		tracker->attacker = attacker.off;
		tracker->damage = damage;
		tracker->target = target;
		tracker->kind = kind;
		tracker->updateChecksum();
		return tracker->fire(attacker, x, y, z, slot, 1);
	}
	return 0;
}

int16_t GetMissileDistance(ItemId startItem, ItemId targetItem)
{
	Coord targetX, targetY, startX, startY;
	objref ref;
	TypeFrame typeFrame;
	int16_t targetZ, startZ;
	Path path;

	ref = objref(targetItem.off);
	typeFrame.bits = ref.ptr()->typeFrame;
	targetX = Item_getX(ref);
	targetX -= GetFootprintX(typeFrame) >> 1;
	targetY = Item_getY(ref);
	targetY -= GetFootprintY(typeFrame) >> 1;
	targetZ = Item_getZ(&ref) + (gItemTypeInfo[typeFrame.bits & 0x3ff].height >> 1);
	if (targetZ > 15)
		targetZ = 15;
	ref = objref(startItem.off);
	typeFrame.bits = ref.ptr()->typeFrame;
	startX = Coord(Item_getX(ref).value - (GetFootprintX(typeFrame) >> 1));
	startY = Coord(Item_getY(ref).value - (GetFootprintY(typeFrame) >> 1));
	startZ = (uint8_t) (Item_getZ(&ref) + gItemTypeInfo[ref.ptr()->typeFrame & 0x3ff].height) - 1;
	if (startZ < 0)
		startZ = 0;
	path.initialize(startX, startY, startZ, targetX, targetY, targetZ);
	return path.initialDistance;
}

int16_t GetMissileDistanceTo(int16_t item, Coord targetX, Coord targetY, int16_t &targetZ)
{
	Coord x, y;
	objref ref;
	TypeFrame typeFrame;
	int16_t startZ;
	Path path;

	ref = objref(item);
	typeFrame.bits = ref.ptr()->typeFrame;
	x = Coord(Item_getX(ref).value - (GetFootprintX(typeFrame) >> 1));
	y = Coord(Item_getY(ref).value - (GetFootprintY(typeFrame) >> 1));
	startZ = (uint8_t)(Item_getZ(&ref) + gItemTypeInfo[ref.ptr()->typeFrame & 0x3ff].height) - 1;
	if (startZ < 0)
		startZ = 0;
	path.initialize(x, y, startZ, targetX, targetY, targetZ);
	return path.initialDistance;
}

void CheckMissilesForItem(ItemId item)
{
	int16_t i;
	uint8_t returning, hit;

	for (i = 0; i < MISSILE_COUNT; i++) {
		if (MissileTrackers[i].active && MissileTrackers[i].weapon) {
			WeaponRef weapon = MissileTrackers[i].weapon;
			WeaponRecord details;
			WeaponRecords.read(weapon.index, &details);
			returning = details.returns;
			if (MissileTrackers[i].ammo) {
				AmmoRef ammo = MissileTrackers[i].ammo;
				AmmoRecord ammoDetails;
				AmmoRecords.read(ammo.index, &ammoDetails);
				if (ammoDetails.returns)
					returning = 1;
			}
			hit = 0;
			if (details.homing && (uint8_t)(MissileTrackers[i].target.off == item.off))
				hit = 1;
			else if (returning && (uint8_t)(MissileTrackers[i].attacker.off == item.off))
				hit = 1;
			if (hit)
				MissileHitThisPass = 1;
		}
	}
}

uint8_t StopMissile(int16_t index)
{
	if (index < 0 || index >= MISSILE_COUNT) {
		CheatPrintf("sm: bad missile %d", index);
		return 1;
	}
	return MissileTrackers[index].stop();
}
