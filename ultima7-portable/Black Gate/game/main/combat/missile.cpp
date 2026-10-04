/* Black Gate U7.EXE, overlay segment 244 (file offsets 0x070090 to 0x07148a, 5114 bytes).
 * Borland C++ 2.0 -mm -O -G -P -Y rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include <new>
#include <stdlib.h>
#include "lowlevel.h"
#include "typefram.h"
#include "iteminfo.h"
#include "dosio.h"
#include "itemrec.h"
#include "objref.h"
#include "datanode.h"
#include "collide.h"
#include "makemojo.h"
#include "u7npc.h"
#include "combwpn.h"
#include "equip.h"
#include "sounds.h"
#include "wihh.h"
#include "easyfile.h"
#include "debug.h"
#include "misstrac.h"
#include "type.h"
#include "item.h"
#include "npcref.h"
#include "coord.h"
#include "ready.h"
#include "combatai.h"
#include "search.h"
#include "damage.h"
#include "weapons.h"
#include "ammo.h"

struct Path {
	uint8_t majorDirection;
	int16_t initialDistance, stepsRemaining, middleError, middleStep, middleCorrection;
	uint8_t middleDirection;
	int16_t minorError, minorStep, minorCorrection;
	uint8_t minorDirection;
	uint8_t state;        /* 0 stopped, 1 arrived, 2 moving, 3 on the target */
	int16_t x, y, z;                /* in eighths of a cell */
	Path() { state = 0; }
	uint8_t status();
	void step(uint8_t direction);
	uint8_t advance();
	void initialize(Coord, Coord, int16_t, Coord, Coord, int16_t);
};
struct MissileTracker : Path {
	uint16_t missile;
	uint8_t active, mode;
	int16_t weapon, ammo;
	ItemId attacker;
	uint8_t damage;
	ItemId target;
	uint8_t kind;         /* speed class; 4 only traces a line of fire */
	int16_t range;
	Coord targetX, targetY;
	uint8_t counted;
	uint32_t storedChecksum;
	void setFrame();
	void home(int16_t);
	void checksum(char *where);
	uint8_t isReturning() { return mode == 2; }
	uint8_t update();
	void updateChecksum() {
		storedChecksum = (uint16_t)(missile + mode + weapon + ammo + attacker.off + damage
			+ target.off + kind + range + targetX.value + targetY.value + counted);
	}
};
struct MissileFile : DataNode {
	virtual char *name();
	virtual void load(char *directory);
	virtual void save(char *directory);
};
struct TypeLookup {
	uint8_t active;
	uint32_t address, sum;
	uint16_t get(uint16_t type) {
		CheckMojoBounds(INT32_C(1024), (int32_t)type);
		return PeekWord(address + type * 2);
	}
};
extern "C" int16_t GetPartyIndex(objref);

char *const MissTracFileName = "MISSTRAC.DAT";
int32_t MissileGuardLow = INT32_C(0xa5a55a5a);  /* guard word; nothing checks it */
MissileTracker MissileTrackers[MISSILE_COUNT];
int32_t MissileGuardHigh = INT32_C(0xa5a55a5a);  /* guard word; nothing checks it */
uint8_t MissileHitThisPass = 0;
uint16_t FirstMissileThisPass = 0;
int16_t ActiveMissiles = 0;
MissileFile MissTracFile;
const int8_t PathStepX[6] = { 1, -1, 0, 0, 0, 0 };
const int8_t PathStepY[6] = { 0, 0, 1, -1, 0, 0 };
const int8_t PathStepZ[6] = { 0, 0, 0, 0, 1, -1 };
const uint8_t MissileFrames[6] = { 12, 20, 16, 8, 22, 14 };
const int8_t MissileFrameTurns[4][4] = {
	{ 0, 0, 1, -1 }, { 0, 0, -1, 1 }, { -1, 1, 0, 0 }, { 1, -1, 0, 0 }
};

char *MissileFile::name()
{
	return MissTracFileName;
}

void MissileFile::save(char *directory)
{
	int16_t file = CreateFileOrFail(BuildPath(directory, MissTracFileName, 0));
	DosWrite(file, -INT32_C(1), (int32_t)sizeof MissileTrackers, MissileTrackers);
	DosClose(file);
}

void MissileFile::load(char *directory)
{
	int16_t file = OpenFileOrFail(BuildPath(directory, MissTracFileName, 0));
	DosRead(file, -INT32_C(1), (int32_t)sizeof MissileTrackers, MissileTrackers);
	DosClose(file);
}

uint8_t Path::status()
{
	if (state == 2 && stepsRemaining <= 0)
		state = 1;
	return state;
}

void Path::initialize(Coord startX, Coord startY, int16_t startZ, Coord targetX, Coord targetY, int16_t targetZ)
{
	int16_t dx, dy, dz;
	int16_t savedDelta, middleDelta, minorDelta;
	uint8_t xDirection, yDirection, zDirection, savedDirection;

	x = startX * 8 + 4;
	y = startY * 8 + 4;
	z = startZ * 8 + 4;
	dx = targetX.value - startX.value;
	xDirection = dx < 0 ? 1 : 0;
	dy = targetY.value - startY.value;
	yDirection = dy < 0 ? 3 : 2;
	dz = targetZ - startZ;
	zDirection = dz < 0 ? 5 : 4;

	stepsRemaining = abs(dx);
	majorDirection = xDirection;
	middleDirection = yDirection;
	middleDelta = abs(dy);
	minorDirection = zDirection;
	minorDelta = abs(dz);

	if (minorDelta > middleDelta) {
		savedDelta = minorDelta;
		savedDirection = minorDirection;
		minorDelta = middleDelta;
		minorDirection = middleDirection;
		middleDelta = savedDelta;
		middleDirection = savedDirection;
	}
	if (stepsRemaining < minorDelta) {
		savedDelta = minorDelta;
		savedDirection = minorDirection;
		minorDelta = stepsRemaining;
		minorDirection = majorDirection;
		stepsRemaining = savedDelta;
		majorDirection = savedDirection;
	}
	if (stepsRemaining < middleDelta) {
		savedDelta = middleDelta;
		savedDirection = middleDirection;
		middleDelta = stepsRemaining;
		middleDirection = majorDirection;
		stepsRemaining = savedDelta;
		majorDirection = savedDirection;
	}

	initialDistance = stepsRemaining;
	middleStep = middleDelta + middleDelta;
	middleError = middleStep - stepsRemaining;
	middleCorrection = middleError - stepsRemaining;
	minorStep = minorDelta + minorDelta;
	minorError = minorStep - stepsRemaining;
	minorCorrection = minorError - stepsRemaining;
	if (stepsRemaining != 0) {
		stepsRemaining <<= 3;
		state = 2;
	} else {
		state = 1;
	}
}

void Path::step(uint8_t direction)
{
	x = (x + PathStepX[direction]) % 24576;
	y = (y + PathStepY[direction]) % 24576;
	z += PathStepZ[direction];
}

uint8_t Path::advance()
{
	if (state == 2) {
		step(majorDirection);
		stepsRemaining--;
		if (middleError < 0)
			middleError += middleStep;
		else {
			step(middleDirection);
			middleError += middleCorrection;
		}
		if (minorError < 0)
			minorError += minorStep;
		else {
			step(minorDirection);
			minorError += minorCorrection;
		}
	}
	return status();
}

void MissileTracker::checksum(char *where)
{
	uint32_t previous;

	if (!active)
		return;
	previous = storedChecksum;
	storedChecksum = (uint16_t)(missile + mode + weapon + ammo + attacker.off + damage
		+ target.off + kind + range + targetX.value + targetY.value + counted);
	if (storedChecksum != previous)
		DebugPrintfAtCoordsWait(1, 23, "MissileTracker checksum failure @ %s!", where);
}

void MissileTracker::setFrame()
{
	uint8_t major, middle, minor;
	int16_t delta, middleRise, minorRise;
	int16_t frame, slope, turn;

	checksum("setFrame 0");
	if (missile != 0) {
		major = majorDirection;
		delta = stepsRemaining >> 3;
		middle = middleDirection;
		middleRise = middleStep;
		minor = minorDirection;
		minorRise = minorStep;
		frame = MissileFrames[major];
		if (major < 4) {
			if (middle < 4) {
				slope = 0;
				if (delta != 0)
					slope = (middleRise << 3) / delta;
				turn = MissileFrameTurns[major][middle];
			} else {
				slope = 0;
				if (delta != 0)
					slope = (minorRise << 3) / delta;
				turn = MissileFrameTurns[major][minor];
			}
			if (slope > 3)
				frame += turn;
			if (slope > 11)
				frame += turn;
			if (frame < 8)
				frame += 16;
			if (frame > 23)
				frame -= 16;
		}
		Item_setFrame(&objref(missile), frame);
	}
}

void MissileTracker::home(int16_t newTarget)
{
	objref ref;
	TypeFrame type;
	Coord startX, startY, endX, endY;
	int16_t endZ, startZ;

	if (kind != 4) {
		checksum("home 0");
		target = newTarget;
		updateChecksum();
		ref = objref(target.off);
		startX = x >> 3;
		startY = y >> 3;
		startZ = z >> 3;
		type.bits = ref.ptr()->typeFrame;
		endX = Item_getX(ref);
		endX -= GetFootprintX(type) >> 1;
		endY = Item_getY(ref);
		endY -= GetFootprintY(type) >> 1;
		endZ = Item_getZ(&ref) + (gItemTypeInfo[type.bits & 0x3ff].height >> 1);
		if (endZ > 15)
			endZ = 15;
		if (startX == endX && startY == endY && startZ == endZ) {
			state = 3;
		} else if (Item_getX(ref) != targetX || Item_getY(ref) != targetY) {
			targetX = Item_getX(ref);
			targetY = Item_getY(ref);
			updateChecksum();
			initialize(startX, startY, startZ, endX, endY, endZ);
		}
	}
}

uint8_t MissileTracker::update()
{
	objref projectile;
	int16_t frameStep, step, steps, chance;
	Coord prevX, prevY, nextX, nextY;
	int16_t nextZ, prevZ;
	uint8_t slot, area, returning, certain, hit, continuous, targetHit;
	AreaSearch found;
	WeaponRecord weaponInfo;
	AmmoRecord ammoInfo;

	checksum("update 0");
	if (active && (state == 2 || state == 3)) {
		if (FirstMissileThisPass == 0) {
			FirstMissileThisPass = missile;
			MissileHitThisPass = 0;
		}
		hit = 0;
		projectile = missile;
		continuous = 0;
		if (weapon != 0) {
			int16_t weaponIndex = weapon;
			WeaponRecords.read(weaponIndex, &weaponInfo);
			area = weaponInfo.passesBlockers;
			returning = weaponInfo.returns;
			certain = weaponInfo.autoHit;
		} else {
			area = 0;
			returning = 0;
			certain = 0;
		}
		if (ammo != 0) {
			int16_t ammoIndex = ammo;
			AmmoRecords.read(ammoIndex, &ammoInfo);
			if (ammoInfo.passesBlockers)
				area = 1;
			if (ammoInfo.returns)
				returning = 1;
			if (ammoInfo.autoHit)
				certain = 1;
			if (ammoInfo.keepOnStop && ammoInfo.removeOnStop)
				continuous = 1;
		}
		if (weapon != 0) {
			if ((weaponInfo.homing || isReturning()) && target.valid()) {
				certain = 1;
				home(target.off);
				if (continuous) {
					if (state != 2 && state != 3)
						state = 2;
				} else if (state == 3) {
					state = 1;
				}
				if (!weaponInfo.frameStep)
					setFrame();
			}
			frameStep = weaponInfo.frameStep;
			if (frameStep != 0 && missile != 0) {
				Item_setFrame(&projectile, ((projectile.frame() + frameStep - 8) & 15) + 8);
			}
		}
		if (kind == 0)
			steps = 4;
		else if (kind == 1)
			steps = 8;
		else
			steps = 16;
		prevX = x >> 3;
		prevY = y >> 3;
		prevZ = z >> 3;
		for (step = 0; step < steps; ++step) {
			advance();
			if (continuous && state == 1)
				state = 3;
			nextX = x >> 3;
			nextY = y >> 3;
			nextZ = z >> 3;
			if (nextZ < 0 || nextZ > 15) {
				state = 0;
				nextZ = 0;
			} else if (prevX != nextX || prevY != nextY || prevZ != nextZ) {
				if (continuous) {
					if (!hit && missile != 0) {
						ApplyHitAtCoords(objref(attacker.off), prevX, prevY, prevZ,
							weapon, ammo, missile);
						hit = 1;
					}
					if (state != 2 && state != 3)
						state = 2;
				}
				if (missile != 0)
					Item_detach(&projectile);
				targetHit = target.valid() && (uint8_t)IsPointInItem(target, nextX, nextY, nextZ);
				if (targetHit || IsBoxBlockedAt(nextX, nextY, nextZ, 1, 1, 1) || BlockedByDoor) {
					if (targetHit)
						state = 1;
					if (state != 1 && !area && !(uint8_t)IsPointInItem(attacker, nextX, nextY, nextZ)) {
						state = 0;
						if (kind != 4) {
							FindItemInArea(&found, nextX, nextY, Coord(nextX + 7), Coord(nextY + 7),
								4, -1, -1, 255);
							while (found.current.valid()) {
								if ((uint8_t)IsPointInItem(found.current, nextX, nextY, nextZ))
									break;
								FindItem(&found);
							}
							if (found.current.valid()) {
								chance = (initialDistance - (stepsRemaining >> 3)) * damage / initialDistance;
								if (RollToWin((uint8_t)chance, NPCRef(found.current.off).combat())) {
									target = found.current;
									state = 1;
									certain = 1;
									updateChecksum();
								} else {
									state = 2;
								}
							}
						}
					}
				}
				if (state != 0) {
					prevX = nextX;
					prevY = nextY;
					prevZ = nextZ;
				} else if (missile != 0) {
					prevZ = FindSupportLevel(prevX, prevY, prevZ,
						&TypeFrame(projectile.ptr()->typeFrame));
				}
				if (missile != 0 && !PlaceItem(&projectile, prevX, prevY, prevZ)) {
					state = 0;
					if (missile == FirstMissileThisPass)
						FirstMissileThisPass = 0;
					return 1;
				}
				if (state == 1 && !isReturning() && weapon != 0) {
					if (target.valid() && target != attacker &&
						(uint8_t)IsPointInItem(target, nextX, nextY, nextZ)) {
						if (certain || !objref(target.off).isNpc()
							|| RollToHit(NPCRef(attacker.off), objref(target.off), 0)) {
							int16_t weaponIndex = weapon;
							if (WeaponRecords.get(weaponIndex)->damage != 0)
								PlaySoundAtItem(1, target);
							ApplyWeaponHit(objref(attacker.off), objref(target.off), weapon, ammo, missile, 0);
							hit = 1;
						} else if (stepsRemaining > 0) {
							target.off = 0;
							updateChecksum();
							state = 2;
						} else {
							stepsRemaining = 0;
							state = 0;
							ApplyHitAtCoords(objref(attacker.off), prevX, prevY, prevZ,
								weapon, ammo, missile);
							hit = 1;
						}
					} else {
						ApplyHitAtCoords(objref(attacker.off), prevX, prevY, prevZ,
							weapon, ammo, missile);
						hit = 1;
					}
				}
			}
			if (continuous && state != 0) {
				state = 2;
				home(target.off);
			}
			if (state != 2)
				break;
		}
		if (!hit && state != 2 && missile != 0) {
			if (state == 1 && target.valid() && target != attacker &&
				(uint8_t)IsPointInItem(target, prevX, prevY, prevZ)) {
				ApplyWeaponHit(objref(attacker.off), objref(target.off), weapon, ammo, missile, 0);
			} else {
				ApplyHitAtCoords(objref(attacker.off), prevX, prevY, prevZ, weapon, ammo, missile);
			}
			hit = 1;
		}
		if (range > 0) {
			--range;
			if (range == 0)
				state = 0;
			updateChecksum();
		}
		if (state != 2 && returning) {
			if (isReturning() && missile != 0) {
				slot = ReadyRecords.get(ReadyLookup.get(projectile.type()))->slot;
				if (!objref(GetItemInSlot(attacker, slot)).valid()) {
					EquipItem(projectile, attacker, slot, 0);
					PartyMissileFlags[GetPartyIndex(NPCRef(attacker.off))] = 0;
				}
			} else {
				home(attacker.off);
				mode = 2;
				updateChecksum();
			}
		}
		if (missile == FirstMissileThisPass) {
			FirstMissileThisPass = 0;
			if (MissileHitThisPass)
				return 1;
		}
	} else {
		DebugPrintf("Illegal missile!");
		return 1;
	}
	return state != 2 && state != 3;
}

uint8_t HasLineOfFireToCoords(ItemId attacker, CellCoord x, CellCoord y, int16_t z)
{
	ItemId noMissile;
	noMissile.off = 0;
	return FireMissileAtCoords(noMissile, 0, 0, attacker, 0, x, y, z, 4);
}

uint8_t HasLineOfFire(ItemId attacker, ItemId target)
{
	ItemId noMissile;
	noMissile.off = 0;
	return FireMissileAtItem(noMissile, 0, 0, attacker, 0, target, 4);
}

uint8_t UpdateMissile(int16_t index)
{
	if (index < 0 || index >= MISSILE_COUNT) {
		CheatPrintf("um: bad missile %d", index);
		return 1;
	} else {
		return MissileTrackers[index].update();
	}
}

extern "C" void ResetMissileGlobals(void)
{
	MissileGuardLow = INT32_C(0xa5a55a5a);
	memset((void *)MissileTrackers, 0, sizeof(MissileTrackers));
	MissileGuardHigh = INT32_C(0xa5a55a5a);
	MissileHitThisPass = 0;
	FirstMissileThisPass = 0;
	ActiveMissiles = 0;
	memset((void *)&MissTracFile, 0, sizeof(MissTracFile));
}

extern "C" void ConstructMissileGlobals(void)
{
	new (&MissTracFile) MissileFile();
}
