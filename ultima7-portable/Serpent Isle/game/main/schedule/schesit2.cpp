/* Serpent Isle SI.EXE, overlay segment 287 (file offsets 0x07c460 to 0x07ce20, 2496 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include "iteminfo.h"
#include "item.h"
#include "coord.h"
#include "u7npc.h"
#include "random.h"
#include "npcpath.h"
#include "actitem.h"
#include "sortitem.h"
#include "search.h"
#include "script.h"

extern int16_t DiscardedPathLength[2];

#define NPC(p) GetNpcBufferForIbo(p)
#define CUR_SCHED(p) (NPC(p)->schedules[NPC(p)->currentSchedule])
/* a seat's frame gives the way it faces, in quarter turns */
#define SEAT_FACING(ref) ((((ITEM((ref).off)->typeFrame & 0x7c00) >> 10) & 3) * 2)

/*
 * Sit: find a chair, or failing that a seat, near an item of the type the first argument names, or
 * with -1 anywhere within twelve cells; walk in front of it, face the way it faces and sit down.
 */
void DoWorkSit(objref *npc)
{
	int8_t result;
	Coord x, y;
	int16_t count, choice, pass, type, i;
	AreaSearch seats, anchor, selected, unusedSearch;

	result = 2;
	type = 873 /* chair */;
	switch (CUR_SCHED(npc).state) {
	case 0:
		if (CUR_SCHED(npc).x == -1) {
			CUR_SCHED(npc).state = 4;
			break;
		}
		for (i = 0; i < 2; i++) {
			FindNearestItem(&anchor, Item_getX(*npc), Item_getY(*npc),
				25, 0, CUR_SCHED(npc).x, 0xff, 0xff);
			FindNearestItem(&seats, Item_getX(anchor.current), Item_getY(anchor.current),
				12, 0, type, 0xff, 0xff);
			if (anchor.found() && seats.found()) {
				x = Coord(Item_getX(seats.current) + DirDeltaX[SEAT_FACING(seats.current)]);
				y = Coord(Item_getY(seats.current) + DirDeltaY[SEAT_FACING(seats.current)]);
				result = StartPath(*npc, x, y, Item_getZ(&seats.current), 100, DiscardedPathLength, 0);
				if (result == 0) {
					NPC(npc)->direction = SEAT_FACING(seats.current);
					CUR_SCHED(npc).state = 8;
					return;
				}
				if (result == 1)
					return;
			}
			type = 292 /* seat */;
		}
		Npc_popSchedule(npc, -1);
		break;
	case 4:
		for (pass = 0; pass < 2; pass++) {
			FindItemInArea(&seats,
				Coord(Item_getX(*npc) - 12), Coord(Item_getY(*npc) - 12),
				Coord(Item_getX(*npc) + 12), Coord(Item_getY(*npc) + 12),
				0, type, 0xff, 0xff);
			selected = seats;
			count = 0;
			while (selected.found()) {
				count++;
				FindItem(&selected);
			}
			choice = GenerateRandomIntegerInRange(count);
			selected = seats;
			for (i = 0; i < choice; i++)
				FindItem(&selected);
			if (selected.found()) {
				x = Coord(Item_getX(selected.current) + DirDeltaX[SEAT_FACING(selected.current)]);
				y = Coord(Item_getY(selected.current) + DirDeltaY[SEAT_FACING(selected.current)]);
				result = StartPath(*npc, x, y, Item_getZ(&selected.current), 100, DiscardedPathLength, 0);
				if (result == 0) {
					NPC(npc)->direction = SEAT_FACING(selected.current);
					CUR_SCHED(npc).state++;
					return;
				}
				if (result == 1)
					return;
			}
			while (seats.found()) {
				x = Coord(Item_getX(seats.current) + DirDeltaX[SEAT_FACING(seats.current)]);
				y = Coord(Item_getY(seats.current) + DirDeltaY[SEAT_FACING(seats.current)]);
				result = StartPath(*npc, x, y, Item_getZ(&seats.current), 100, DiscardedPathLength, 0);
				if (result == 0) {
					NPC(npc)->direction = SEAT_FACING(seats.current);
					CUR_SCHED(npc).state++;
					return;
				}
				if (result == 1)
					return;
				FindItem(&seats);
			}
			type = 292 /* seat */;
		}
		Npc_popSchedule(npc, -1);
		break;
	case 5:
		ContinueScheduleWalk(npc);
		break;
	case 6:
		if (IsBlocked(npc))
			CUR_SCHED(npc).state = 4;
		else
			CUR_SCHED(npc).state = 9;
		break;
	case 8:
		ContinueScheduleWalk(npc);
		break;
	case 9:
		if (IsBlocked(npc))
			Npc_popSchedule(npc, -1);
		else
			PostScheduleScript(npc, MakeScript(SCRIPT_FACE, NPC(npc)->direction + 48,
				SCRIPT_STAND_FRAME, SCRIPT_WAIT, 2, SCRIPT_BEND_FRAME, SCRIPT_WAIT, 2, SCRIPT_SIT_FRAME, SCRIPT_WAIT, 2,
				SCRIPT_END));
		break;
	case 10:
		Npc_popSchedule(npc, 0);
		break;
	}
}
