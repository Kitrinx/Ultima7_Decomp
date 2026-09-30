/* Black Gate U7.EXE, overlay segment 285 (file offsets 0x085670 to 0x085864, 500 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder inferred from compiler flags and link order.
 */

#include "activity.h"
#include "iteminfo.h"
#include "item.h"
#include "coord.h"
#include "u7npc.h"
#include "random.h"
#include "npcpath.h"
#include "actitem.h"

extern objref AvatarRef;
extern int DiscardedPathLength[2];

/* the header's coordinate form, overloaded for an item */
inline int Item_greatestDeltaToCoords(objref *item, objref other)
{
	return Item_greatestDeltaToItem(*item, other);
}

#define NPC(p) GetNpcBufferForIbo(p)
#define CUR_SCHED(p) (NPC(p)->schedules[NPC(p)->currentSchedule])

/* Near the avatar, step to a random spot within five cells that lies farther from him. */
void far RunShySchedule(objref *npc)
{
	char result;
	Coord x, y;
	int distance;

	switch (CUR_SCHED(npc).state) {
	case 0:
		Npc_pushSchedule(npc, WORK_GOTO_SCH_D, -1, -1);
		break;
	case 1:
		distance = Item_greatestDeltaToCoords(npc, AvatarRef);
		if (distance <= 10) {
			x = Coord(GenerateRandomIntegerInRange(11) + Item_getX(*npc).value - 5);
			y = Coord(GenerateRandomIntegerInRange(11) + Item_getY(*npc).value - 5);
			if (Item_greatestDeltaToCoords(AvatarRef, x, y, Item_getZ(npc)) > distance + 2) {
				result = StartPath(*npc, x, y, Item_getZ(npc), 100, DiscardedPathLength, 0);
				if (result == 0)
					CUR_SCHED(npc).state++;
			}
		}
		break;
	case 2:
		ContinueScheduleWalk(npc);
		break;
	case 3:
		CUR_SCHED(npc).state = 1;
		break;
	}
}
