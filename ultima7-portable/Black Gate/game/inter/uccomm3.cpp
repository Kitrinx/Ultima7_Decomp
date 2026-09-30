/* Black Gate U7.EXE, overlay segment 318 (file offsets 0x0931b0 to 0x0936bb, 1291 bytes).
 * Borland C++ 2.0 -mm -O -P -d rebuilds it byte for byte as C++.
 */

/* path: ..\inter\uccomm3.c */
#include "u7port.h"
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

extern uint8_t StartAutoroute(Coord x, Coord y, int8_t z, int16_t silent, int16_t callback, int16_t argument, int16_t event);
extern void SetRouteFailureUsecode(int16_t, int16_t, int16_t);

#define UCCOMM_ERROR(line)  AssertFail(__FILE__, line)

/* the int in element i of the usecode value at v */
#define ARG(v, i)   GetListNode(v, i)->toInt()

#define NPC(p)  GetNpcBufferForIbo(p)
#define CUR_SCHED(p)    (NPC(p)->schedules[NPC(p)->currentSchedule])

/* Usecode engine calls: args - 1 is the first argument, ret takes the result. */

/* 0x03: show an NPC's face */
void UC_SetSpeaker(Value *args, Value *)
{
	int16_t npc, face, frame;

	npc = ARG(args - 1, 1);
	if (npc == UC_AVATAR)
		face = 0;
	else if (npc < 0 && npc > UC_AVATAR)
		face = -npc;
	else
		UCCOMM_ERROR(147);
	frame = ARG(args - 2, 1);
	ShowSpeaker(face, frame);
}

/* 0x91: bring back the first speaker's face */
void UC_ResetConvFace(Value *, Value *)
{
	ShowFirstSpeaker();
}

/* 0x04: remove an NPC's face */
void UC_CloseSpeaker(Value *args, Value *)
{
	int16_t npc, face;

	npc = ARG(args - 1, 1);
	if (npc == UC_AVATAR)
		face = 0;
	else if (npc < 0 && npc > UC_AVATAR)
		face = -npc;
	else
		UCCOMM_ERROR(188);
	RemoveSpeaker(face);
}

/* 0x0a: the answer picked, as text */
void UC_GetInput(Value *, Value *ret)
{
	RunOptionsLoop();
	ret->appendFarString(ChosenAnswer);
}

/* 0x0b: the answer picked, as its place among the answers */
void UC_GetOrdInput(Value *, Value *ret)
{
	Answer *a;
	int16_t i;

	RunOptionsLoop();
	a = 0;
	i = 0;
	while (OfferedAnswers.next(&a)) {
		i++;
		if (strcmp(ChosenAnswer, a->text) == 0)
			ret->appendInt(i);
	}
}

/* 0x0c: ask for a number */
void UC_InputNumericValue(Value *args, Value *ret)
{
	int16_t value = 0;
	int16_t low, high, step, initial;

	low = ARG(args - 1, 1);
	high = ARG(args - 2, 1);
	step = ARG(args - 3, 1);
	initial = ARG(args - 4, 1);
	value = GetSliderValue(low, high, step, initial, 104, 57);
	ret->appendInt(value);
}

/* 0x46: sit an NPC down */
void UC_SitDown(Value *args, Value *)
{
	int16_t obj = GetItemRef(GetListNode(args - 1, 1));
	objref r = obj;

	Npc_setSchedule(&r, WORK_MAJOR_SIT);
	CUR_SCHED(&r).state = 1;
	PartySitRefs[GetPartyIndex(r)] = ARG(args - 2, 1);
}

/* 0x1c: an NPC's schedule */
void UC_GetWorkType(Value *args, Value *ret)
{
	int16_t obj = GetItemRef(GetListNode(args - 1, 1));
	objref r = obj;

	ret->appendInt(NPC(&r)->workType);
}

/* 0x1d: set an NPC's schedule */
void UC_SetWorkType(Value *args, Value *)
{
	int16_t obj = GetItemRef(GetListNode(args - 1, 1));
	objref r = obj;
	int8_t type = ARG(args - 2, 1);

	if ((int8_t)Item_isAvatar(&r) && NPC(&r)->workType == WORK_COMBAT && type != WORK_COMBAT)
		BreakOffCombat();
	else {
		Npc_setSchedule(&r, type);
		CUR_SCHED(&r).state = 1;
	}
}

/* 0x7d: walk the avatar to a position, then run a usable for an item */
void UC_PathRunUsecode(Value *args, Value *ret)
{
	Coord x = ARG(args - 1, 1);
	Coord y = ARG(args - 1, 2);
	int8_t z = ARG(args - 1, 3);
	int16_t usable = ARG(args - 2, 1);
	int16_t item = ARG(args - 3, 1);
	int16_t event = ARG(args - 4, 1);
	int16_t started = StartAutoroute(x, y, z, 1, usable, item, event);

	ret->appendInt(started);
}

/* 0x8b: the usable to run for an item when the walk fails */
void UC_SetPathFailure(Value *args, Value *)
{
	int16_t usable, item, event;

	usable = ARG(args - 1, 1);
	item = ARG(args - 2, 1);
	event = ARG(args - 3, 1);
	SetRouteFailureUsecode(usable, item, event);
}
