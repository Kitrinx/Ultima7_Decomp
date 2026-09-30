/* Black Gate U7.EXE, overlay segment 206 (file offsets 0x04eec0 to 0x050756, 6294 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include "objref.h"
#include "iteminfo.h"
#include "activity.h"
#include "coord.h"
#include "u7npc.h"
#include "wihh.h"
#include "random.h"
#include "npcpath.h"
#include "actutil.h"
#include "script.h"
#include "item.h"
#include "type.h"
#include "actmove.h"
#include "actqueue.h"
#include "legalmov.h"
#include "movepath.h"
#include "sche.h"
#include "scheserv.h"
#include "search.h"
#include "sortitem.h"
#include "sounds.h"
#include "mapview.h"
#include "actitem.h"

extern objref AvatarRef;
#define NPC(ref) GetNpcBufferForIbo(ref)
#define CURRENT(ref) (NPC(ref)->schedules[NPC(ref)->currentSchedule])

/* The schedule handlers, indexed by work type */
WorkHandler WorkTypeHandlers[53] = {
	0, RunPaceSchedule, RunPaceSchedule, RunTalkSchedule, RunDanceSchedule, RunEatSchedule, RunFarmSchedule,
	(WorkHandler) RunTendShopSchedule, RunMinerSchedule, DoWorkHound, RunStandSchedule,
	(WorkHandler) RunLoiterSchedule, (WorkHandler) RunWanderSchedule, RunBlacksmithSchedule, RunSleepSchedule,
	(WorkHandler) RunWaitSchedule, RunMajorSitSchedule, RunGrazeSchedule, RunBakeSchedule, RunSewSchedule,
	RunShySchedule, RunLabSchedule, RunThiefSchedule, RunWaiterSchedule, (WorkHandler) RunSpecialSchedule,
	RunKidGamesSchedule, RunEatAtInnSchedule, RunDuelSchedule, RunPreachSchedule, RunPatrolSchedule,
	RunDeskWorkSchedule, 0, WalkToItemSchedule, PickUpItemSchedule, PutDownItemSchedule, DoWorkSit, DoWorkFillBucket,
	DoWorkRead, DoWorkPayRespects, DoWorkFillWaterTrough, DoWorkArchery, DoWorkFencing, DoWorkReadyHand,
	PauseSchedule, CarryItemSchedule, DoWorkCheckLight, DoWorkTag, BoardBargeSchedule, ArrestAvatarSchedule,
	WalkToScheduleSchedule, WalkToSpotSchedule, DoWorkCheckShutters, DoWorkCheckArea
};
inline uint8_t HasNoSchedule(objref *ref) { return NPC(ref)->currentSchedule == 0; }
inline uint16_t GetHeight(objref *ref) { return gItemTypeInfo[ref->ptr()->typeFrame & 0x3ff].height; }

objref GetBackpackItem(objref ref)
{
	return objref(GetItemInSlot(ref, 0));
}

objref GetWeaponHandItem(objref ref)
{
	return objref(GetItemInSlot(ref, 1));
}

objref GetOffHandItem(objref ref)
{
	return objref(GetItemInSlot(ref, 2));
}

/* Try spots in widening rings around the Avatar until one fits the NPC and it can path to the
 * destination from there. Puts the NPC back where it was when none works. */
uint8_t PlaceNpcNearAvatar(objref *ref, Loc destinationX, Loc destinationY, uint8_t keepPath)
{
	int16_t radius, direction, firstDirection;
	Coord avatarX, avatarY, x, y, oldX, oldY, oldZ;
	uint8_t result;

	oldX = Item_getX(*ref);
	oldY = Item_getY(*ref);
	oldZ = Item_getZ(ref);
	avatarX = Item_getX(AvatarRef);
	avatarY = Item_getY(AvatarRef);
	firstDirection = GenerateRandomIntegerInRange(8) & ~1;
	for (radius = 0; radius < 10; radius += 2)
		for (direction = firstDirection; direction < firstDirection + 8; direction += 2) {
			x = avatarX + DirDeltaX[direction & 7] * (radius + 24);
			y = avatarY + DirDeltaY[direction & 7] * (radius + 24);
			if (CanTypeMoveTo(x, y, 0, ref->ptr()->typeFrame)) {
				Item_move(ref, CellCoord(x), CellCoord(y));
				result = StartPath(*ref, destinationX, destinationY, 0, 100, DiscardedPathLength, 0);
				if (result == 0) {
					if (!keepPath)
					StopPaths(*ref);
					return 1;
				}
			}
		}
	Item_move(ref, oldX, oldY, oldZ);
	return 0;
}

void PostScheduleScript(objref *ref, char *code)
{
	CURRENT(ref).state++;
	ActionQueue.add(ref->off, code);
}

void PostScriptToItem(objref *ref, char *code)
{
	if (CanVisit(ref) && ref->valid()) {
		ActionQueue.remove(ref->off, 1, 0);
		ActionQueue.add(ref->off, code);
	}
}

uint8_t ContinueScheduleWalk(objref *ref)
{
	uint8_t result = WalkNPCRoute(*ref, 1);
	if (result == 0) {
		ActionQueue.run(ActionQueue.add(ref->off, (char *)MakeScript(SCRIPT_STAND_FRAME, SCRIPT_END)));
		CURRENT(ref).state++;
	} else if (result == 2) {
		if (HasNoSchedule(ref))
			NPC(ref)->result = -1;
		else
			Npc_popSchedule(ref, -1);
	}
	return result;
}

/* Pack a type and frame into one word; frame 255 leaves the frame unset. */
uint16_t MakeTypeFrame(uint16_t type, uint16_t frame)
{
	TypeFrame result(0);
	result.setType(type);
	if (frame != 255) {
		result.setFrame(frame);
		result.bits |= 0x8000;
	}
	return result.bits;
}

void WalkToItemSchedule(objref *ref)
{
	uint8_t result;
	uint8_t retry = 0;
	if (CURRENT(ref).state & 0x80) {
		retry = 1;
		CURRENT(ref).state = CURRENT(ref).state & 0x7f;
	}
	switch (CURRENT(ref).state) {
	case 0:
		result = StartWalkToItem(ref, TypeFrame(CURRENT(ref).x),
			TypeFrame(CURRENT(ref).y), retry);
		if (result == 0)
			++CURRENT(ref).state;
		else
			Npc_popSchedule(ref, -1);
		break;
	case 1:
		ContinueScheduleWalk(ref);
		break;
	case 2:
		if (IsBlocked(ref))
			Npc_popSchedule(ref, -1);
		else
			PostScheduleScript(ref, MakeScript(SCRIPT_FACE, NPC(ref)->direction + 0x30, SCRIPT_END));
		break;
	case 3:
		Npc_popSchedule(ref, CURRENT(ref).x & 0x3ff);
		break;
	}
}

void PickUpItemSchedule(objref *ref)
{
	objref item;
	TypeFrame typeFrame;
	int16_t frame;
	AreaSearch found;
	typeFrame.bits = CURRENT(ref).x;
	if (typeFrame.flipped())
		frame = typeFrame.frame();
	else
		frame = 255;
	switch (CURRENT(ref).state) {
	case 0:
		item = FindCarriedItem(ref, TypeFrame(CURRENT(ref).x));
		if (item.valid())
			Npc_popSchedule(ref, 0);
		else
			Npc_pushSchedule(ref, WORK_GOTO_ITEM, CURRENT(ref).x, -1);
		break;
	case 1:
		if (IsBlocked(ref)) {
			if (CURRENT(ref).y == 1) {
				item = CreateCarriedItem(TypeFrame(CURRENT(ref).x), *ref);
				if (item.valid())
					Npc_popSchedule(ref, 0);
				else
					Npc_popSchedule(ref, -1);
			} else
				Npc_popSchedule(ref, -1);
		} else {
			FindNearestItem(&found, Item_getX(*ref), Item_getY(*ref),
				8, 0, typeFrame.type(), 255, frame);
			if (Item_getZ(&found.current) >= Item_getZ(ref) + 2) {
				CURRENT(ref).counter = 1;
				PostScheduleScript(ref, MakeScript(SCRIPT_STAND_FRAME, SCRIPT_WAIT, 1, SCRIPT_READY_FRAME,
					SCRIPT_WAIT, 2, SCRIPT_END));
			} else {
				CURRENT(ref).counter = 0;
				PostScheduleScript(ref, MakeScript(SCRIPT_STAND_FRAME, SCRIPT_WAIT, 2, SCRIPT_BEND_FRAME,
					SCRIPT_WAIT, 2, SCRIPT_END));
			}
		}
		break;
	case 2:
		FindNearestItem(&found, Item_getX(*ref), Item_getY(*ref),
			8, 0, typeFrame.type(), 255, frame);
		if (found.current.valid()) {
			Item_moveIntoContainer(&found.current, *ref);
			++CURRENT(ref).state;
		} else
			Npc_popSchedule(ref, -1);
		break;
	case 3:
		if (CURRENT(ref).counter)
			PostScheduleScript(ref, MakeScript(SCRIPT_READY_FRAME, SCRIPT_WAIT, 1, SCRIPT_STAND_FRAME, SCRIPT_WAIT, 2,
				SCRIPT_END));
		else
			PostScheduleScript(ref, MakeScript(SCRIPT_BEND_FRAME, SCRIPT_WAIT, 2, SCRIPT_STAND_FRAME, SCRIPT_WAIT, 2,
				SCRIPT_END));
		break;
	case 4:
		Npc_popSchedule(ref, 0);
		break;
	}
}

void PutDownItemSchedule(objref *ref)
{
	objref item;
	TypeFrame wanted;
	DropSpot step;
	Coord x, y;
	AreaSearch nearby;
	uint16_t frame;
	wanted.bits = CURRENT(ref).y;
	if (wanted.flipped())
		frame = wanted.frame();
	else
		frame = 255;

	switch (CURRENT(ref).state) {
	case 0:
		if (CURRENT(ref).y == -1)
			CURRENT(ref).counter = 0;
		else
			CURRENT(ref).counter = 1;
		if (CURRENT(ref).y == -1) {
			step = FindDropSpot(ref, TypeFrame(CURRENT(ref).x).type());
			if (step.direction == 8) {
				Npc_popSchedule(ref, -1);
				break;
			}
			NPC(ref)->direction = (uint16_t)step.direction & 6;
			CURRENT(ref).y = step.z;
			NPC(ref)->scheduleValue = step.direction;
			if (Item_getZ(ref) + 2 <= (uint16_t)(step.z))
				PostScheduleScript(ref, MakeScript(SCRIPT_FACE, NPC(ref)->direction + 0x30,
					SCRIPT_STAND_FRAME, SCRIPT_WAIT, 1, SCRIPT_READY_FRAME, SCRIPT_WAIT, 2, SCRIPT_END));
			else
				PostScheduleScript(ref, MakeScript(SCRIPT_FACE, NPC(ref)->direction + 0x30,
					SCRIPT_STAND_FRAME, SCRIPT_WAIT, 2, SCRIPT_BEND_FRAME, SCRIPT_WAIT, 2, SCRIPT_END));
		} else
			CURRENT(ref).state = 4;
		break;
	case 1:
		x = Item_getX(*ref) + DirDeltaX[NPC(ref)->scheduleValue];
		y = Item_getY(*ref) + DirDeltaY[NPC(ref)->scheduleValue];
		item = FindCarriedItem(ref, TypeFrame(CURRENT(ref).x));
		if (item.valid() && CanTypeMoveTo(x, y, CURRENT(ref).y, wanted.type())) {
			++CURRENT(ref).state;
			Item_move(&item, x, y, CURRENT(ref).y);
			PlaySoundAtItem(18, item);
		} else
			Npc_popSchedule(ref, -1);
		break;
	case 2:
		if (CURRENT(ref).counter && (uint16_t)CURRENT(ref).y >= Item_getZ(ref) + 2)
			PostScheduleScript(ref, MakeScript(SCRIPT_READY_FRAME, SCRIPT_WAIT, 1, SCRIPT_STAND_FRAME, SCRIPT_WAIT, 2,
				SCRIPT_END));
		else
			PostScheduleScript(ref, MakeScript(SCRIPT_BEND_FRAME, SCRIPT_WAIT, 2, SCRIPT_STAND_FRAME, SCRIPT_WAIT, 2,
				SCRIPT_END));
		break;
	case 3:
		Npc_popSchedule(ref, 0);
		break;
	case 4:
		Npc_pushSchedule(ref, WORK_GOTO_ITEM, CURRENT(ref).y, CURRENT(ref).x);
		CURRENT(ref).state = 0x80;
		break;
	case 5:
		if (IsBlocked(ref)) {
			Npc_popSchedule(ref, -1);
			break;
		}
		FindNearestItem(&nearby, Item_getX(*ref), Item_getY(*ref),
			8, 0, wanted.type(), 255, frame);
		if (nearby.current.valid()) {
			CURRENT(ref).y = Item_getZ(&nearby.current) + GetHeight(&nearby.current);
			NPC(ref)->scheduleValue = GetFacing(ref);
			if ((uint16_t)CURRENT(ref).y >= Item_getZ(ref) + 2)
				PostScheduleScript(ref, MakeScript(SCRIPT_STAND_FRAME, SCRIPT_WAIT, 2, SCRIPT_READY_FRAME,
					SCRIPT_WAIT, 1, SCRIPT_END));
			else
				PostScheduleScript(ref, MakeScript(SCRIPT_STAND_FRAME, SCRIPT_WAIT, 2, SCRIPT_BEND_FRAME,
					SCRIPT_WAIT, 2, SCRIPT_END));
			CURRENT(ref).state = 1;
		} else
			Npc_popSchedule(ref, -1);
		break;
	}
}

void CarryItemSchedule(objref *ref)
{
	objref item;
	switch (CURRENT(ref).state) {
	case 0:
		item = FindCarriedItem(ref, TypeFrame(CURRENT(ref).x));
		if (item.valid())
			++CURRENT(ref).state;
		else
			CURRENT(ref).state = 5;
		break;
	case 1:
		Npc_pushSchedule(ref, WORK_DROP_ITEM, CURRENT(ref).x, CURRENT(ref).y);
		break;
	case 2:
		if (IsBlocked(ref))
			Npc_popSchedule(ref, -1);
		else
			Npc_popSchedule(ref, 0);
		break;
	case 5:
		Npc_pushSchedule(ref, WORK_GRAB_ITEM, CURRENT(ref).x, 1);
		break;
	case 6:
		item = FindCarriedItem(ref, TypeFrame(CURRENT(ref).x));
		if (item.valid())
			CURRENT(ref).state = 1;
		else
			Npc_popSchedule(ref, -1);
		break;
	}
}

/* Find the wanted item, near the reference item when one is given, and start the NPC walking to
 * a spot from which it can use it. Returns 2 when no item or no spot is found. */
uint8_t StartWalkToItem(objref *ref, TypeFrame &wanted, TypeFrame &nearType, uint8_t retry)
{
	AreaSearch target, nearby;
	uint8_t result = 2;
	Coord x, y;
	int16_t z;
	uint8_t direction;
	uint16_t wantedType = wanted.type();
	uint16_t wantedFrame;
	if (wanted.flipped())
		wantedFrame = wanted.frame();
	else
		wantedFrame = 255;
	uint16_t referenceType = nearType.type();
	uint16_t referenceFrame;
	uint8_t found;
	if (nearType.flipped())
		referenceFrame = nearType.frame();
	else
		referenceFrame = 255;

	if (nearType.bits == 0xffff || retry)
		FindNearestItem(&target, Item_getX(*ref), Item_getY(*ref), 25, 0,
			wantedType, 255, wantedFrame);
	else {
		FindNearestItem(&nearby, Item_getX(*ref), Item_getY(*ref), 25, 0x100,
			referenceType, 255, referenceFrame);
		if (nearby.current.valid())
			FindNearestItem(&target, Item_getX(nearby.current), Item_getY(nearby.current), 25, 0x100,
				wantedType, 255, wantedFrame);
	}
	if (target.current.valid()) {
		x = Item_getX(target.current);
		y = Item_getY(target.current);
		z = Item_getZ(&target.current);
		if (retry)
			found = UseSpotFinder.findUseSpot(ref, target.current, &x.value, &y.value, &z, &direction,
				referenceType);
		else
			found = UseSpotFinder.findUseSpot(ref, target.current, &x.value, &y.value, &z, &direction, -1);
		if (found) {
			result = StartPath(*ref, x, y, z, 100, DiscardedPathLength, 0);
			NPC(ref)->direction = direction;
		}
	}
	return result;
}
