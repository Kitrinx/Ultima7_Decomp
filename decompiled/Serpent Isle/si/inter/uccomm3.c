/* Serpent Isle SI.EXE, overlay segment 318 (file offsets 0x08c6b0 to 0x08d111, 2657 bytes).
 * Borland C++ 2.0 -mm -O -P -d rebuilds it byte for byte as C++.
 */

/* path: uccomm3.c */
#include <string.h>
#include "activity.h"
#include "item.h"
#include "u7npc.h"
#include "ucvalue.h"
#include "uclist.h"
#include "convmgr.h"
#include "colbuf.h"
#include "keywords.h"
#include "gumpmgr.h"
#include "init.h"
#include "combat.h"
#include "party.h"
#include "coord.h"
#include "sche.h"

extern unsigned char far StartAutoroute(Coord x, Coord y, char z, int silent, int callback, int argument, int event,
	char discard);
extern void SetRouteFailureUsecode(int, int, int);
extern void far PathfindNPC(int number, Coord x, Coord y, char z, unsigned char event, int item, int func,
	unsigned char flag);

#define UCCOMM_ERROR(line)  AssertFail(__FILE__, line)

/* the int in element i of the usecode value at v */
#define ARG(v, i)   GetListNode(v, i)->toInt()

#define NPC(p)  GetNpcBufferForIbo(p)
#define CUR_SCHED(p)    (NPC(p)->schedules[NPC(p)->currentSchedule])

/* Usecode engine calls: args - 1 is the first argument, ret takes the result. */

/* 0x03: show an NPC's face */
void far UC_SetSpeaker(Value *args, Value *)
{
	int npc, face, frame;

	npc = ARG(args - 1, 1);
	if (npc == UC_AVATAR)
		face = 0;
	else if (npc < 0 && npc > UC_AVATAR)
		face = -npc;
	else
		UCCOMM_ERROR(162);
	frame = ARG(args - 2, 1);
	ShowSpeaker(face, frame);
}

/* 0xad: bring back the first speaker's face */
void far UC_ResetConvFace(Value *, Value *)
{
	ShowFirstSpeaker();
}

/* 0x04: remove an NPC's face */
void far UC_CloseSpeaker(Value *args, Value *)
{
	int npc, face;

	npc = ARG(args - 1, 1);
	if (npc == UC_AVATAR)
		face = 0;
	else if (npc < 0 && npc > UC_AVATAR)
		face = -npc;
	else
		UCCOMM_ERROR(203);
	RemoveSpeaker(face);
}

/* 0x05: show an NPC's face in the first slot */
void far UC_SetSpeaker0(Value *args, Value *)
{
	int npc, face, frame;

	npc = ARG(args - 1, 1);
	if (npc == UC_AVATAR)
		face = 0;
	else if (npc < 0 && npc > UC_AVATAR)
		face = -npc;
	else
		UCCOMM_ERROR(226);
	frame = ARG(args - 2, 1);
	ShowSpeaker0(face, frame);
}

/* 0x06: show an NPC's face in the second slot */
void far UC_SetSpeaker1(Value *args, Value *)
{
	int npc, face, frame;

	npc = ARG(args - 1, 1);
	if (npc == UC_AVATAR)
		face = 0;
	else if (npc < 0 && npc > UC_AVATAR)
		face = -npc;
	else
		UCCOMM_ERROR(247);
	frame = ARG(args - 2, 1);
	ShowSpeaker1(face, frame);
}

/* 0x07: remove the face in the first slot */
void far UC_CloseSpeaker0(Value *, Value *)
{
	RemoveSpeaker0();
}

/* 0x08: remove the face in the second slot */
void far UC_CloseSpeaker1(Value *, Value *)
{
	RemoveSpeaker1();
}

/* 0x09: which slot speaks next */
void far UC_SetSpeakerSlot(Value *args, Value *)
{
	int slot = ARG(args - 1, 1);

	SetSpeakerSlot(slot);
}

/* 0x0a: change the frame of the face in the first slot */
void far UC_ChangeNpcFace0(Value *args, Value *)
{
	int frame = ARG(args - 1, 1);

	ChangeSpeaker0(frame);
}

/* 0x0b: change the frame of the face in the second slot */
void far UC_ChangeNpcFace1(Value *args, Value *)
{
	int frame = ARG(args - 1, 1);

	ChangeSpeaker1(frame);
}

/* 0x11: the answer picked, as text */
void far UC_GetInput(Value *, Value *ret)
{
	RunOptionsLoop();
	ret->appendFarString(ChosenAnswer);
}

/* 0x12: the answer picked, as its place among the answers */
void far UC_GetOrdInput(Value *, Value *ret)
{
	Answer *a;
	int i;

	RunOptionsLoop();
	a = 0;
	i = 0;
	while (OfferedAnswers.next(&a)) {
		i++;
		if (strcmp(ChosenAnswer, a->text) == 0)
			ret->appendInt(i);
	}
}

/* 0x13: ask for a number */
void far UC_InputNumericValue(Value *args, Value *ret)
{
	int value = 0;
	int low, high, step, initial;

	low = ARG(args - 1, 1);
	high = ARG(args - 2, 1);
	step = ARG(args - 3, 1);
	initial = ARG(args - 4, 1);
	value = GetSliderValue(low, high, step, initial, 104, 57);
	ret->appendInt(value);
}

/* 0x54: sit an NPC down */
void far UC_SitDown(Value *args, Value *)
{
	int obj = GetItemRef(GetListNode(args - 1, 1));
	objref r = obj;

	if (r.valid()) {
		Npc_setSchedule(&r, WORK_MAJOR_SIT);
		CUR_SCHED(&r).state = 1;
		PartySitRefs[GetPartyIndex(r)] = ARG(args - 2, 1);
	}
}

/* 0x24: an NPC's schedule */
void far UC_GetWorkType(Value *args, Value *ret)
{
	int obj = GetItemRef(GetListNode(args - 1, 1));
	objref r = obj;

	if (r.valid())
		ret->appendInt(NPC(&r)->workType);
	else
		ret->appendInt(0);
}

/* 0x25: set an NPC's schedule */
void far UC_SetWorkType(Value *args, Value *)
{
	int obj = GetItemRef(GetListNode(args - 1, 1));
	objref r = obj;
	char type = ARG(args - 2, 1);

	if (r.valid()) {
		if ((char)Item_isAvatar(&r) && NPC(&r)->workType == WORK_COMBAT && type != WORK_COMBAT)
			BreakOffCombat();
		else {
			Npc_setSchedule(&r, type);
			CUR_SCHED(&r).state = 1;
		}
	}
}

/* 0xc1: walk an NPC to a place, then run a usable for an item */
void far UC_PathfindNPC(Value *args, Value *)
{
	int npc = GetItemRef(GetListNode(args - 1, 1));
	Coord x = ARG(args - 2, 1);
	Coord y = ARG(args - 2, 2);
	char z = ARG(args - 2, 3);
	unsigned char event = ARG(args - 3, 1);
	int item = ARG(args - 4, 1);
	int func = ARG(args - 5, 1);
	unsigned char flag = ARG(args - 6, 1);

	PathfindNPC(npc, x, y, z, event, item, func, flag);
}

/* 0x94: walk the avatar to a position, then run a usable for an item */
void far UC_PathRunUsecode(Value *args, Value *ret)
{
	Coord x = ARG(args - 1, 1);
	Coord y = ARG(args - 1, 2);
	char z = ARG(args - 1, 3);
	int usable = ARG(args - 2, 1);
	int item = ARG(args - 3, 1);
	int event = ARG(args - 4, 1);
	int started = StartAutoroute(x, y, z, 1, usable, item, event, 0);

	ret->appendInt(started);
}

/* 0x95: reads an NPC, a position and three numbers, and does nothing */
void far UC_UnusedItemWalk(Value *args, Value *)
{
	int npc = GetItemRef(GetListNode(args - 1, 1));
	Coord x = ARG(args - 2, 1);
	Coord y = ARG(args - 2, 2);
	char z = ARG(args - 2, 3);
	int usable = ARG(args - 3, 1);
	int item = ARG(args - 4, 1);
	int event = ARG(args - 5, 1);
}

/* 0x96: whether the avatar can walk to a position */
void far UC_CanAvatarReach(Value *args, Value *ret)
{
	Coord x = ARG(args - 1, 1);
	Coord y = ARG(args - 1, 2);
	char z = ARG(args - 1, 3);
	int usable = 0;
	int item = 0;
	int event = 0;
	int reachable = StartAutoroute(x, y, z, 1, usable, item, event, 1);

	ret->appendInt(reachable);
}

/* 0x97: reads a position and a number, and does nothing */
void far UC_UnusedWalk(Value *args, Value *)
{
	Coord x = ARG(args - 1, 1);
	Coord y = ARG(args - 1, 2);
	char z = ARG(args - 1, 3);
	int usable = 0;
	int item = ARG(args - 2, 1);
	int event = 0;
}

/* 0xa7: the usable to run for an item when the walk fails */
void far UC_SetPathFailure(Value *args, Value *)
{
	int usable, item, event;

	usable = ARG(args - 1, 1);
	item = ARG(args - 2, 1);
	event = ARG(args - 3, 1);
	SetRouteFailureUsecode(usable, item, event);
}
