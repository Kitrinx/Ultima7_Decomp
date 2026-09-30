/* Black Gate U7.EXE, overlay segment 207 (file offsets 0x0508f0 to 0x0519ce, 4318 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Original folder unknown.
 */

#include "u7port.h"
#include "itemrec.h"
#include "iteminfo.h"
#include "lowlevel.h"
#include "activity.h"
#include "type.h"
#include "npcref.h"
#include "coord.h"
#include "u7npc.h"
#include "actitem.h"
#include "combatai.h"
#include "makemojo.h"
#include "missile.h"
#include "monsters.h"
#include "sche.h"
#include "sche_ov1.h"
#include "random.h"
#include "text.h"
#include "sprite.h"
#include "usehook.h"
#include "npcpath.h"
#include "party.h"
#include "combat.h"
#include "sortitem.h"
#include "script.h"
#include "attack.h"
#include "partymov.h"
#include "voolook.h"
#include "search.h"
#include "actmove.h"

#define NPC(p) GetNpcBufferForIbo(p)
#define CUR_SCHED(p) (NPC(p)->schedules[NPC(p)->currentSchedule])

extern uint8_t Item_moveIntoContainer(objref *ref, objref container);
extern Coord Item_getX(objref &);
extern Coord Item_getY(objref &);
extern uint8_t Item_getQualityFlags(objref *);
extern "C" int16_t Item_greatestDeltaToItem(objref &, objref);
extern uint8_t Item_move(objref *, CellCoord, CellCoord);
extern int16_t DiscardedPathLength[2];

inline uint8_t GetScheduleKind(objref *p) { return CUR_SCHED(p).kind; }
inline uint8_t GetScheduleState(objref *p) { return CUR_SCHED(p).state; }

int8_t StowHeldItems(objref *ref)
{
	objref held, bag, destination;
	bag = GetBackpackItem(objref(ref->off));
	destination = bag.valid() && bag.isContainer() ? bag.off : ref->off;
	held = GetOffHandItem(objref(ref->off));
	if (held.valid())
		Item_moveIntoContainer(&held, destination);
	held = GetWeaponHandItem(objref(ref->off));
	if (held.valid()) {
		Item_moveIntoContainer(&held, destination);
		return 1;
	}
	return 0;
}

void PauseSchedule(objref *ref)
{
	switch (CUR_SCHED(ref).state) {
	case 0:
		PostScheduleScript(ref, MakeScript(SCRIPT_WAIT, CUR_SCHED(ref).x, SCRIPT_END));
		break;
	case 1:
		Npc_popSchedule(ref, 0);
		break;
	}
}

void ArrestAvatarSchedule(objref *ref)
{
	int16_t monsterIndex;
	uint16_t type;

	switch (CUR_SCHED(ref).state) {
	case 0:
		ContinueScheduleWalk(ref);
		break;
	case 1:
		type = objref(ref->off).type();
		CheckMojoBounds(INT32_C(1024), (int32_t)type);
		monsterIndex = PeekWord(MonsterLookup.addr + type * 2);
		if ((uint8_t)(Item_getQualityFlags(&AvatarRef) & 1)
			&& !(uint8_t)MonsterRecords.get(monsterIndex)->seeInvisible
			|| !PathNextToItem(ref, AvatarRef, 75)) {
			CUR_SCHED(ref).state = 0;
			Npc_pushSchedule(ref, WORK_LOITER, -1, -1);
			CUR_SCHED(ref).state = 1;
		} else {
			if ((uint16_t)NPC(ref)->primaryTarget != (uint16_t)Item_getNpcNumber(&AvatarRef)) {
				Npc_popSchedule(ref, -1);
				return;
			}
			if (Item_greatestDeltaToItem(*ref, AvatarRef) <= 2 && HasLineOfFire(*ref, AvatarRef))
				CUR_SCHED(ref).state++;
			else if (RollChance(35))
				SpriteManager_barkOnItem(&gSpriteManager, *ref,
					GetGameText(1, GenerateRandomIntegerInRange(3) + 23), 0, 15, 0);
		}
		break;
	case 2:
		PostScheduleScript(ref, MakeScript(SCRIPT_STAND_FRAME, SCRIPT_END));
		break;
	case 3:
		CUR_SCHED(ref).state++;
		RunUsable(1, *ref, 0x625);
		break;
	case 4:
		Npc_setSchedule(ref, WORK_LOITER);
		CUR_SCHED(ref).state = 1;
		break;
	}
}

void WalkToSpotSchedule(objref *ref)
{
	switch (CUR_SCHED(ref).state) {
	case 0:
		if (StartPath(*ref, NPC(ref)->sVr[0], NPC(ref)->sVr[1], Item_getZ(ref), 100,
				DiscardedPathLength, 0) == 0)
			CUR_SCHED(ref).state++;
		else
			Npc_popSchedule(ref, -1);
		break;
	case 1:
		ContinueScheduleWalk(ref);
		break;
	case 2:
		Npc_popSchedule(ref, IsBlocked(ref) ? -1 : 0);
		break;
	}
}

void BoardBargeSchedule(objref *ref)
{
	Coord x, y;
	uint8_t result;
	objref leader;
	int16_t member, count, ready;
	objref *members;
	int16_t i;

	switch (CUR_SCHED(ref).state) {
	case 0:
		CUR_SCHED(ref).counter = 0;
		CUR_SCHED(ref).state++;
		break;
	case 1:
		member = GetPartyIndex(*ref);
		leader = objref(PartySitRefs[member]);
		x = Coord(Item_getX(leader) + DirDeltaX[(leader.frame() & 3) * 2]);
		y = Coord(Item_getY(leader) + DirDeltaY[(leader.frame() & 3) * 2]);
		result = StartPath(*ref, x, y, Item_getZ(&leader), 100, DiscardedPathLength, 0);
		if (result != 0) {
			NPC(ref)->typeFlagsHigh = NPC(ref)->typeFlagsHigh | 8;
			CUR_SCHED(ref).state = 3;
			return;
		}
		NPC(ref)->direction = (leader.frame() & 3) << 1;
		CUR_SCHED(ref).state++;
		break;
	case 2:
		ContinueScheduleWalk(ref);
		break;
	case 3:
		if (IsBlocked(ref)) {
			if (CUR_SCHED(ref).counter < 3) {
				CUR_SCHED(ref).counter++;
				CUR_SCHED(ref).state = 0;
				Npc_pushSchedule(ref, WORK_PAUSE, 5, -1);
			} else
				Npc_popSchedule(ref, -1);
		} else
			CUR_SCHED(ref).state++;
		break;
	case 4:
		PostScheduleScript(ref, MakeScript(SCRIPT_FACE, NPC(ref)->direction + 0x30,
			SCRIPT_STAND_FRAME, SCRIPT_WAIT, 2, SCRIPT_BEND_FRAME, SCRIPT_WAIT, 2, SCRIPT_SIT_FRAME, SCRIPT_WAIT, 2,
			SCRIPT_END));
		break;
	case 5:
		ready = 0;
		members = PartyMembers;
		count = PartySize;
		for (i = 0; i < count; i++)
			if (GetScheduleKind(&members[i]) == WORK_HACK_SIT && GetScheduleState(&members[i]) == 6)
				ready++;
		if (count - 1 == ready) {
			CUR_SCHED(ref).state++;
			RunUsable(1, AvatarRef, 0x634);
		} else
			CUR_SCHED(ref).state++;
		break;
	}
}

void WalkToScheduleSchedule(objref *ref)
{
	Coord targetX, targetY, x, y;
	int8_t state;
	uint8_t result;
	int16_t targetZ;
	int16_t step = 14;
	AreaSearch search;

	if (IsInParty(ref))
		Npc_popSchedule(ref, 0);
	switch (CUR_SCHED(ref).state) {
	case 0:
		switch (NPC(ref)->workType) {
		case WORK_HOR_PACE:
		case WORK_VER_PACE:
			SelectWeapon(*ref, 0, 0);
			break;
		default:
			StowHeldItems(ref);
		}
		/* fall through */
	case 1:
		state = ContinueScheduleWalk(ref);
		if (state != 0)
			return;
		if (IsAtScheduleLocation(ref, &targetX, &targetY)) {
			Npc_popSchedule(ref, 0);
			return;
		}
		if (StartPath(*ref, targetX, targetY, 0, 150, DiscardedPathLength, 0)) {
		retry:
			x = Item_getX(*ref);
			y = Item_getY(*ref);
			if (x != targetX) {
				if (x < targetX) {
					x += step;
					if (x > targetX)
						x = targetX;
				} else {
					x -= step;
					if (x < targetX)
						x = targetX;
				}
			}
			if (y != targetY) {
				if (y < targetY) {
					y += step;
					if (y > targetY)
						y = targetY;
				} else {
					y -= step;
					if (y < targetY)
						y = targetY;
				}
			}
			result = StartPath(*ref, x, y, 0, 150, DiscardedPathLength, 0);
			if (result != 2)
				return;
			if (step >= 6) {
				step -= 4;
				goto retry;
			} else {
				if (GetDistance(Item_getX(AvatarRef), Item_getY(AvatarRef), Item_getZ(&AvatarRef),
						targetX, targetY, 0) >= 24
					&& GetDistance(Item_getX(AvatarRef), Item_getY(AvatarRef), Item_getZ(&AvatarRef),
						Item_getX(*ref), Item_getY(*ref), Item_getZ(ref)) >= 24)
					Item_move(ref, targetX, targetY);
				else if (GetDistance(Item_getX(*ref), Item_getY(*ref), Item_getZ(ref),
						targetX, targetY, 0) <= 5)
					Npc_popSchedule(ref, 0);
				else {
					FindItemInArea(&search, x, y, x, y, 32, -1, 255, 255);
					if (search.found())
						if (FindSpotNextToItem(ref, search.current, &x, &y, &targetZ)) {
							result = StartPath(*ref, x, y, targetZ, 150, DiscardedPathLength, 0);
							if (result == 0 || result == 1)
								return;
						}
					Npc_setSchedule(ref, WORK_WANDER);
					CUR_SCHED(ref).state = 1;
				}
			}
		} else
			return;
		break;
	case 2:
		CUR_SCHED(ref).state = 1;
		break;
	}
}

int8_t IsAtScheduleLocation(objref *ref, Coord *x, Coord *y)
{
	if ((uint16_t)Item_getNpcNumber(ref) >= 256)
		return 1;
	if (Schedule_getCoord(&ScheduleTable, (uint16_t)Item_getNpcNumber(ref), SchedulePeriod, x, y)
		&& Item_getX(*ref) == *x && Item_getY(*ref) == *y)
		return 1;
	return 0;
}
