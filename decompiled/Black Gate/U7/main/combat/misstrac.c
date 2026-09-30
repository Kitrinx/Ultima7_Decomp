/* Black Gate U7.EXE, overlay segment 245 (file offsets 0x071510 to 0x072876, 4966 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

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
	unsigned char majorDirection;
	int initialDistance, stepsRemaining, middleError, middleStep, middleCorrection;
	unsigned char middleDirection;
	int minorError, minorStep, minorCorrection;
	unsigned char minorDirection, state;
	int x, y, z;
	Path() { state = 0; }
	void far initialize(Coord startX, Coord startY, int startZ, Coord targetX, Coord targetY, int targetZ);
};
struct MissileTracker : Path {
	unsigned missile;
	unsigned char active, mode;
	int weapon, ammo;
	objref attacker;
	unsigned char damage;
	objref target;
	unsigned char kind;
	int range;
	Coord targetX, targetY;
	unsigned char counted;
	unsigned long storedChecksum;
	void far checksum(char *where);
	void far setFrame();
	unsigned char far update();
	unsigned char far stop();
	unsigned char far fire(ItemId, Coord, Coord, int, int, unsigned char);
	void updateChecksum() {
		storedChecksum = missile + mode + weapon + ammo + attacker.off + damage
			+ target.off + kind + range + targetX.value + targetY.value + counted;
	}
};
extern "C" int far GetPartyIndex(objref *);
struct WeaponRef {
	int index;
	WeaponRef(int n) { index = n; }
};
struct AmmoRef {
	int index;
	AmmoRef(int n) { index = n; }
};
struct Script { unsigned char length, bytes[127]; Script() { length = 1; } };
inline Coord GetX(objref ref) { return Item_getX(ref); }
inline Coord GetY(objref ref) { return Item_getY(ref); }

inline void MakeTakeable(objref ref) { Item_setOkayToTake(&ref); }

unsigned char far MissileTracker::fire(ItemId attacker,
	Coord aimX, Coord aimY, int aimZ, int slot, unsigned char directional)
{
	Coord x, y;
	int z;
	objref actor;
	TypeFrame typeFrame;
	unsigned char stopped, returning, area;
	int reach;

	checksum("fire 0");
	if ((unsigned char)(ItemTypeClassFlags[gItemTypeInfo[objref(missile).type()].typeClass] & CLASS_QUALITY_FLAGS)) {
		DebugPrintfAtCoordsWait(1, 1, "Try to fire xtra status msl type %d", objref(missile).type());
		active = 0;
		return 0;
	}
	if ((unsigned char)Item_hasHitPoints(&objref(missile))) {
		DebugPrintfAtCoordsWait(1, 1, "Try to fire breakable msl type %d", objref(missile).type());
		active = 0;
		return 0;
	}
	counted = 0;
	targetX = GetX(target);
	targetY = GetY(target);
	updateChecksum();
	actor = objref(attacker.off);
	typeFrame.bits = actor.ptr()->typeFrame;
	x = Coord((int)Item_getX(actor) - (GetFootprintX(typeFrame) >> 1));
	y = Coord((int)Item_getY(actor) - (GetFootprintY(typeFrame) >> 1));
	z = (unsigned char)(Item_getZ(&actor) + gItemTypeInfo[actor.type()].height) - 1;
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
		int weaponIndex = weapon;
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
				int ammoIndex = ammo;
				AmmoRecords.read(ammoIndex, &ammunition);
				if (ammunition.returns)
					returning = 1;
				if (ammunition.passesBlockers)
					area = 1;
			}
			if (returning) {
				PartyMissileFlags[GetPartyIndex(&NPCRef(attacker.off))] = 1;
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

unsigned char far MissileTracker::stop()
{
	objref ref;
	unsigned char remove = 0;
	unsigned char retainOnStop, removeOnStop;
	if (active) {
		checksum("stop 0");
		if (missile) {
			if (missile == DetachedItems)
				remove = 1;
			else if (weapon) {
				ref = objref(missile);
				Item_setFrame(&ref, 0);
				WeaponRecord weaponInfo;
				int weaponIndex = weapon;
				WeaponRecords.read(weaponIndex, &weaponInfo);
				retainOnStop = 0;
				removeOnStop = 0;
				if (weaponInfo.ammo == -3 && ammo)
					retainOnStop = 1;
				else if (weaponInfo.ammo == -1 || weaponInfo.ammo == -2 || weaponInfo.explodes)
					removeOnStop = 1;
				if (ammo) {
					AmmoRecord ammoInfo;
					int ammoIndex = ammo;
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
	} else {
		DebugPrintf("Tried to stop unused missile %d!", this - MissileTrackers);
	}
	return remove;
}

int far AllocMissileTracker()
{
	int slot = MISSILE_COUNT;
	int i;
	for (i = 0; i < MISSILE_COUNT; i++) {
		if (!MissileTrackers[i].active) {
			MissileTrackers[i].active = 1;
			slot = i;
			break;
		}
	}
	return slot;
}

void far ResetMissileTrackers()
{
	int i;
	for (i = 0; i < MISSILE_COUNT; i++)
		MissileTrackers[i].active = 0;
	ActiveMissiles = 0;
}

unsigned char far FireMissileAtCoords(ItemId missile, int weapon, int ammo, ItemId attacker,
	unsigned char damage, Coord targetX, Coord targetY, int targetZ, unsigned char kind)
{
	int slot;
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

unsigned char far FireMissileAtItem(ItemId missile, int weapon, int ammo, ItemId attacker,
	unsigned char damage, ItemId target, unsigned char kind)
{
	Coord x, y;
	int z, slot;
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

unsigned char far FireMissileInDirection(ItemId missile, int weapon, int ammo, ItemId attacker,
	unsigned char damage, unsigned char direction, unsigned char kind)
{
	Coord x, y;
	int z;
	objref target, ref;
	TypeFrame typeFrame;
	int slot;
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
		x = Coord(Coord((int)Item_getX(ref) - (GetFootprintX(typeFrame) >> 1)) + DirDeltaX[direction]);
		y = Coord(Coord((int)Item_getY(ref) - (GetFootprintY(typeFrame) >> 1)) + DirDeltaY[direction]);
		z = (unsigned char)(Item_getZ(&ref) + gItemTypeInfo[ref.type()].height) - 1;
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

int far GetMissileDistance(ItemId startItem, ItemId targetItem)
{
	Coord targetX, targetY, startX, startY;
	objref ref;
	TypeFrame typeFrame;
	int targetZ, startZ;
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
	startZ = (unsigned char) (Item_getZ(&ref) + gItemTypeInfo[ref.ptr()->typeFrame & 0x3ff].height) - 1;
	if (startZ < 0)
		startZ = 0;
	path.initialize(startX, startY, startZ, targetX, targetY, targetZ);
	return path.initialDistance;
}

int far GetMissileDistanceTo(int item, Coord targetX, Coord targetY, int &targetZ)
{
	Coord x, y;
	objref ref;
	TypeFrame typeFrame;
	int startZ;
	Path path;

	ref = objref(item);
	typeFrame.bits = ref.ptr()->typeFrame;
	x = Coord(Item_getX(ref).value - (GetFootprintX(typeFrame) >> 1));
	y = Coord(Item_getY(ref).value - (GetFootprintY(typeFrame) >> 1));
	startZ = (unsigned char)(Item_getZ(&ref) + gItemTypeInfo[ref.ptr()->typeFrame & 0x3ff].height) - 1;
	if (startZ < 0)
		startZ = 0;
	path.initialize(x, y, startZ, targetX, targetY, targetZ);
	return path.initialDistance;
}

void far CheckMissilesForItem(ItemId item)
{
	int i;
	unsigned char returning, hit;

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
			if (details.homing && (unsigned char)(MissileTrackers[i].target.off == item.off))
				hit = 1;
			else if (returning && (unsigned char)(MissileTrackers[i].attacker.off == item.off))
				hit = 1;
			if (hit)
				MissileHitThisPass = 1;
		}
	}
}

unsigned char far StopMissile(int index)
{
	if (index < 0 || index >= MISSILE_COUNT) {
		CheatPrintf("sm: bad missile %d", index);
		return 1;
	}
	return MissileTrackers[index].stop();
}
