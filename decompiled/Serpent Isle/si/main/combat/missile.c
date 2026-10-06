/* Serpent Isle SI.EXE, overlay segment 357 (file offsets 0x0b1c70 to 0x0b305f, 5103 bytes).
 * Borland C++ 2.0 -mm -O -G -P -Y rebuilds it byte for byte as C++.
 */

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
	unsigned char majorDirection;
	int initialDistance, stepsRemaining, middleError, middleStep, middleCorrection;
	unsigned char middleDirection;
	int minorError, minorStep, minorCorrection;
	unsigned char minorDirection;
	unsigned char state;        /* 0 stopped, 1 arrived, 2 moving, 3 on the target */
	int x, y, z;                /* in eighths of a cell */
	Path() { state = 0; }
	unsigned char far status();
	void far step(unsigned char direction);
	unsigned char far advance();
	void far initialize(Coord, Coord, int, Coord, Coord, int);
};
struct MissileTracker : Path {
	unsigned missile;
	unsigned char active, mode;
	int weapon, ammo;
	ItemId attacker;
	unsigned char damage;
	ItemId target;
	unsigned char kind;         /* speed class; 4 only traces a line of fire */
	int range;
	Coord targetX, targetY;
	unsigned char counted;
	unsigned long storedChecksum;
	void far setFrame();
	void far home(int);
	void far checksum(char *where);
	unsigned char isReturning() { return mode == 2; }
	unsigned char far update();
	void updateChecksum() {
		storedChecksum = missile + mode + weapon + ammo + attacker.off + damage
			+ target.off + kind + range + targetX.value + targetY.value + counted;
	}
};
struct MissileFile : DataNode {
	virtual char *name();
	virtual void load(char *directory);
	virtual void save(char *directory);
};
struct TypeLookup {
	unsigned char active;
	unsigned long address, sum;
	unsigned get(unsigned type) {
		CheckMojoBounds(1024L, (long)type);
		return PeekWord(address + type * 2);
	}
};
extern "C" int far GetPartyIndex(objref *);

char *MissTracFileName = "MISSTRAC.DAT";
long MissileGuardLow = 0xa5a55a5aL;  /* guard word; nothing checks it */
MissileTracker MissileTrackers[MISSILE_COUNT];
long MissileGuardHigh = 0xa5a55a5aL;  /* guard word; nothing checks it */
unsigned char MissileHitThisPass = 0;
unsigned FirstMissileThisPass = 0;
int ActiveMissiles = 0;
MissileFile MissTracFile;
signed char PathStepX[6] = { 1, -1, 0, 0, 0, 0 };
signed char PathStepY[6] = { 0, 0, 1, -1, 0, 0 };
signed char PathStepZ[6] = { 0, 0, 0, 0, 1, -1 };
unsigned char MissileFrames[6] = { 12, 20, 16, 8, 22, 14 };
signed char MissileFrameTurns[4][4] = {
	{ 0, 0, 1, -1 }, { 0, 0, -1, 1 }, { -1, 1, 0, 0 }, { 1, -1, 0, 0 }
};

char *MissileFile::name()
{
	return MissTracFileName;
}

void MissileFile::save(char *directory)
{
	int file = CreateFileOrFail(BuildPath(directory, MissTracFileName, 0));
	DosWrite(file, -1L, (long)sizeof MissileTrackers, MissileTrackers);
	DosClose(file);
}

void MissileFile::load(char *directory)
{
	int file = OpenFileOrFail(BuildPath(directory, MissTracFileName, 0));
	DosRead(file, -1L, (long)sizeof MissileTrackers, MissileTrackers);
	DosClose(file);
}

unsigned char far Path::status()
{
	if (state == 2 && stepsRemaining <= 0)
		state = 1;
	return state;
}

void far Path::initialize(Coord startX, Coord startY, int startZ, Coord targetX, Coord targetY, int targetZ)
{
	int dx, dy, dz;
	int savedDelta, middleDelta, minorDelta;
	unsigned char xDirection, yDirection, zDirection, savedDirection;

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

void far Path::step(unsigned char direction)
{
	x = (x + PathStepX[direction]) % 24576;
	y = (y + PathStepY[direction]) % 24576;
	z += PathStepZ[direction];
}

unsigned char far Path::advance()
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

void far MissileTracker::checksum(char *where)
{
	unsigned long previous;

	if (!active)
		return;
	previous = storedChecksum;
	storedChecksum = missile + mode + weapon + ammo + attacker.off + damage
		+ target.off + kind + range + targetX.value + targetY.value + counted;
	if (storedChecksum != previous)
		DebugPrintfAtCoordsWait(1, 23, "MissileTracker checksum failure @ %s!", where);
}

void far MissileTracker::setFrame()
{
	unsigned char major, middle, minor;
	int delta, middleRise, minorRise;
	int frame, slope, turn;

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

void far MissileTracker::home(int newTarget)
{
	objref ref;
	TypeFrame type;
	Coord startX, startY, endX, endY;
	int endZ, startZ;

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

unsigned char far MissileTracker::update()
{
	objref projectile;
	int frameStep, step, steps, chance;
	Coord prevX, prevY, nextX, nextY;
	int nextZ, prevZ;
	unsigned char slot, area, returning, certain, hit, continuous, targetHit;
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
			int weaponIndex = weapon;
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
			int ammoIndex = ammo;
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
				targetHit = target.valid() && (unsigned char)IsPointInItem(target, nextX, nextY, nextZ);
				if (targetHit || IsBoxBlockedAt(nextX, nextY, nextZ, 1, 1, 1) || BlockedByDoor) {
					if (targetHit)
						state = 1;
					if (state != 1 && !area && !(unsigned char)IsPointInItem(attacker, nextX, nextY, nextZ)) {
						state = 0;
						if (kind != 4) {
							FindItemInArea(&found, nextX, nextY, Coord(nextX + 7), Coord(nextY + 7),
								4, -1, -1, 255);
							while (found.current.valid()) {
								if ((unsigned char)IsPointInItem(found.current, nextX, nextY, nextZ))
									break;
								FindItem(&found);
							}
							if (found.current.valid()) {
								chance = (initialDistance - (stepsRemaining >> 3)) * damage / initialDistance;
								if (RollToWin((unsigned char)chance, NPCRef(found.current.off).combat())) {
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
						(unsigned char)IsPointInItem(target, nextX, nextY, nextZ)) {
						if (certain || !objref(target.off).isNpc()
							|| RollToHit(NPCRef(attacker.off), objref(target.off), 0)) {
							int weaponIndex = weapon;
							if (WeaponRecords.get(weaponIndex)->damage != 0)
								PlaySoundAtItem(80, target);
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
				(unsigned char)IsPointInItem(target, prevX, prevY, prevZ)) {
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
					PartyMissileFlags[GetPartyIndex(&NPCRef(attacker.off))] = 0;
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
		return 1;
	}
	return state != 2 && state != 3;
}

unsigned char far HasLineOfFireToCoords(ItemId attacker, CellCoord x, CellCoord y, int z)
{
	ItemId noMissile;
	noMissile.off = 0;
	return FireMissileAtCoords(noMissile, 0, 0, attacker, 0, x, y, z, 4);
}

unsigned char far HasLineOfFire(ItemId attacker, ItemId target)
{
	ItemId noMissile;
	noMissile.off = 0;
	return FireMissileAtItem(noMissile, 0, 0, attacker, 0, target, 4);
}

unsigned char far UpdateMissile(int index)
{
	if (index < 0 || index >= MISSILE_COUNT) {
		CheatPrintf("um: bad missile %d", index);
		return 1;
	} else {
		return MissileTrackers[index].update();
	}
}
