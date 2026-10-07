/* Serpent Isle SI.EXE, overlay segment 349 (file offsets 0x0a8780 to 0x0a920c, 2700 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

/* path: combmode.c */
#include "u7port.h"
#include "lowlevel.h"
#include "activity.h"
#include "iteminfo.h"
#include "item.h"
#include "npcref.h"
#include "voolook.h"
#include "u7npc.h"
#include "combatai.h"
#include "combat.h"
#include "sprite.h"
#include "npcpath.h"
#include "combpick.h"
#include "movepath.h"
#include "random.h"
#include "makemojo.h"
#include "missile.h"
#include "monsters.h"
#include "text.h"
#include "combmode.h"

#define NPC(p) GetNpcBufferForIbo(p)
#define CUR_SCHED(p) NPC(p)->schedules[NPC(p)->currentSchedule]
inline uint8_t IsProtecting(objref *p) { return Item_getQuality(p) == ATTACK_PROTECT; }
inline uint8_t IsInGroup(objref *p) { return CombatGroups.leaders[GetAlignment(p)] != -1; }

const int16_t SummonTypes[8] = {528, 514, 532, 337, 501, 530, 517, 661};
const uint8_t SummonCounts[8] = {4, 4, 2, 1, 1, 4, 8, 3};
const uint8_t SummonOdds[8] = {50, 65, 69, 73, 77, 89, 95, 100};
extern "C" void PlaySfx(uint8_t number, uint16_t volume, int16_t pan);
extern uint8_t CanItemMoveTo(CellCoord, CellCoord, int16_t, ItemId);
/* damage type 1 is fire */
inline uint8_t IsImmuneToFire(int16_t type)
{
	return (MonsterRecords.get(type)->immune >> 1) & 1;
}

struct MonsterRef {
	int16_t index;
	MonsterRef() {}
	MonsterRef(int16_t n) { index = n; }
	MonsterRecord *operator->() { return MonsterRecords.get(index); }
	uint8_t none() { return index == 0; }
};

uint8_t IsTargeting(objref *actor, objref other)
{
	if (Item_getNpcNumber(&other) == NPC(actor)->primaryTarget ||
		Item_getNpcNumber(&other) == NPC(actor)->secondaryTarget)
		return 1;
	return 0;
}

void UseSecondSpot(objref *actor)
{
	NPC(actor)->iVr[0] = NPC(actor)->sVr[0];
	NPC(actor)->iVr[1] = NPC(actor)->sVr[1];
	NPC(actor)->sVr[0] = -1;
}

int8_t PathToSpot(objref *actor, Loc x, Loc y, int16_t z, int16_t distance)
{
	int16_t result;
	int8_t status = StartPath(*actor, x, y, z, distance, &result, 1);

	if (status != 0)
		return 0;
	if (WalkNPCRoute(*actor, 1) == 2)
		return 0;
	StopPaths(*actor);
	return 1;
}

int8_t TeleportInCombat(objref *actor, uint8_t fleeing)
{
	Coord originX, originY, x, y, oldX, oldY;
	int16_t z, oldZ, skill;
	objref target;
	int16_t type;
	int16_t attempt;

	GetNpcIbo(&target, GetCombatTarget(actor));
	if (fleeing) {
		originX = Item_getX(*actor);
		originY = Item_getY(*actor);
		z = Item_getZ(actor);
	} else {
		originX = Item_getX(AvatarRef);
		originY = Item_getY(AvatarRef);
		z = Item_getZ(&AvatarRef);
	}
	for (attempt = 0; attempt < 4; attempt++) {
		if (fleeing) {
			x = GenerateRandomIntegerInRange(17) + originX - 8;
			y = GenerateRandomIntegerInRange(9) + originY - 4;
		} else {
			x = GenerateRandomIntegerInRange(35) + originX - 17;
			y = GenerateRandomIntegerInRange(17) + originY - 8;
		}
		if (fleeing && !(Item_greatestDeltaToItem(*actor, target) <
			Item_greatestDeltaToCoords(target, x, y, z)))
			continue;
		if (CanItemMoveTo(x, y, z, *actor) == 0)
			continue;
		if (fleeing == 0 && HasLineOfFireToCoords(target, x, y, z) == 0)
			continue;
		if (StartPath(*actor, x, y, (int8_t)z, 60, DiscardedPathLength, 0) != 0)
			continue;
		StopPaths(*actor);
		skill = Npc_getIntelligence(actor);
		type = GetMonsterNumber(*actor);
		oldX = Item_getX(*actor);
		oldY = Item_getY(*actor);
		oldZ = Item_getZ(actor);
		Item_move(actor, x, y, z);
		if (IsImmuneToFire(type))
			CombatGroups.makeArrivalEffect(oldX, oldY, oldZ, -1, -1);
		else
			CombatGroups.makeArrivalEffect(oldX, oldY, oldZ, skill + 10, skill * 2 + 10);
		SpriteManager_playSpriteForItem(&gSpriteManager, *actor, 0, 0, 0, 0, 1031, 0, -1, 5);
		PlaySfx(2, 255, 64);
		return 1;
	}
	return 0;
}

int8_t SummonMonsters(objref *actor)
{
	int16_t index, count, roll, side;

	side = GetAlignment(actor);
	roll = GenerateRandomIntegerInRange(100);
	for (index = 0; index < 8; index++) {
		if (SummonOdds[index] > roll)
			break;
	}
	count = SummonCounts[index];
	if (count != 1) {
		if (RollChance(2))
			count += count / 2;
		else
			count -= count / 2;
	}
	objref target;
	GetNpcIbo(&target, GetCombatTarget(actor));
	/* skeletons rise from the ground */
	if (CombatGroups.spawnGroup(SummonTypes[index], side == 0 ? 3 : side, count, 0, 10,
		0, 0, SummonTypes[index] == 528 ? (int8_t)1 : (int8_t)0, target, 0)) {
		SpriteManager_playSpriteForItem(&gSpriteManager, *actor, 0, 0, 0, 0, 1037, 0, -1, 5);
		if (SummonTypes[index] == 528)
			PlaySfx(39, 255, 64);
		else
			PlaySfx(130, 255, 64);
	}
	return 1;
}

void SetAttackMode(NPCRef &actor, int8_t mode)
{
	MonsterRef monster;
	NPC(&actor)->iVr[0] = -1;
	if (!IsInCombat(&actor)) {
		Npc_setSchedule(&actor, WORK_COMBAT);
		CUR_SCHED(&actor).state = 1;
	}
	if (IsProtecting(&actor) && IsInGroup(&actor))
		ReassignProtector(&actor);
	monster.index = GetMonsterNumber(actor);
	if (mode == ATTACK_PROTECT) {
		NPC(&actor)->typeFlagsHigh = NPC(&actor)->typeFlagsHigh | 1;
		if (RollChance(3)) {
			switch (actor.type()) {
			case 354: case 478: case 691: case 725: case 744:
				SpriteManager_barkOnItem(&gSpriteManager, actor, GetGameText(1, 211), 5, 15, 0);
				break;
			default:
				if (!(monster->extraFlags & 0x20))
					SpriteManager_barkOnItem(&gSpriteManager, actor,
						GetGameText(1, GenerateRandomIntegerInRange(3) + 52), 5, 15, 0);
				break;
			}
		}
	} else if (mode == ATTACK_FLEE && !Npc_hasTournamentFlag(&actor)) {
		switch (actor.type()) {
		case 354: case 478: case 691: case 725: case 744:
			SpriteManager_barkOnItem(&gSpriteManager, actor, GetGameText(1, 212), 5, 15, 0);
			break;
		default:
			if (!(monster->extraFlags & 0x20))
				SpriteManager_barkOnItem(&gSpriteManager, actor, GetGameText(1, 55), 5, 15, 0);
			break;
		}
	}
	CombatGroups.movePoints[Item_getNpcNumber(&actor)] = 0;
	Item_setQuality(&actor, mode);
	RemoveFromCombat(actor);
}

void SetAttackModeByRef(objref *actor, int16_t mode)
{
	SetAttackMode(*actor, mode);
}

void SetNpcOppressor(objref attacker, objref *target)
{
	NPC(&NPCRef(*target))->oppressor = Item_getNpcNumber(&attacker);
}

void SetNpcSecondSpotY(objref *actor, Coord y)
{
	NPC(actor)->iVr[1] = y;
}
