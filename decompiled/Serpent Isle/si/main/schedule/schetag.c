/* Serpent Isle SI.EXE, overlay segment 294 (file offsets 0x07ea80 to 0x07eff6, 1398 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "item.h"
#include "coord.h"
#include "u7npc.h"
#include "sprite.h"
#include "random.h"
#include "combatai.h"
#include "npcpath.h"
#include "actitem.h"
#include "text.h"
#include "script.h"
#include "npcref.h"
#include "search.h"

extern int DiscardedPathLength[2];

#define NPC(p) GetNpcBufferForIbo(p)
#define CUR_SCHED(p) (NPC(p)->schedules[NPC(p)->currentSchedule])

inline char IsSameNPC(NPCRef a, NPCRef b)
{
	return a.off == b.off;
}

/* Pick someone within ten cells, walk up to them, bark a line and gesture; then maybe move on. */
void far DoWorkTag(objref *npc)
{
	AreaSearch pick, npcs;
	int count = 0;
	int choice;
	int z;
	objref target;
	Coord x, y;
	int i;

	switch (CUR_SCHED(npc).state) {
	case 0:
		FindItemInArea(&npcs, Coord(Item_getX(*npc) - 10), Coord(Item_getY(*npc) - 10),
			Coord(Item_getX(*npc) + 10), Coord(Item_getY(*npc) + 10), 4, -1, 0xff, 0xff);
		pick = npcs;
		while (pick.found()) {
			count++;
			FindItem(&pick);
		}
		if (count == 1)
			Npc_popSchedule(npc, -1);
		else {
			pick = npcs;
			choice = GenerateRandomIntegerInRange(count);
			for (i = 0; i < choice; i++)
				FindItem(&pick);
			if (IsSameNPC((objref &)pick.current, *(NPCRef *)npc))
				break;
			NPC(npc)->scheduleValue = Item_getNpcNumber(&NPCRef((objref &)pick.current));
			CUR_SCHED(npc).state++;
		}
		break;
	case 1:
		GetNpcIbo(&target, NPC(npc)->scheduleValue);
		if (!target.valid())
			Npc_popSchedule(npc, -1);
		else {
			if (FindSpotNextToItem(npc, target, &x, &y, &z)) {
				if (StartPath(*npc, x, y, z, 30, DiscardedPathLength, 0) == 0)
					CUR_SCHED(npc).state++;
				else
					Npc_popSchedule(npc, -1);
			} else
				Npc_popSchedule(npc, -1);
		}
		break;
	case 2:
		ContinueScheduleWalk(npc);
		break;
	case 3:
		GetNpcIbo(&target, NPC(npc)->scheduleValue);
		if (target.valid() && Item_greatestDeltaToItem(*npc, target) <= 2 && Npc_getIntelligence(npc) > 5) {
			/* "Catch me if thou canst!", "Tag!  Thou art it!" and three more */
			SpriteManager_barkOnItem(&gSpriteManager, *npc,
				GetGameText(1, GenerateRandomIntegerInRange(5) + 128), 0, 15, 0);
			CUR_SCHED(npc).state++;
		} else
			CUR_SCHED(npc).state = 1;
		break;
	case 4:
		PostScheduleScript(npc, MakeScript(SCRIPT_STAND_FRAME, SCRIPT_WAIT, 5, SCRIPT_END));
		break;
	case 5:
		if (RollChance(4))
			Npc_popSchedule(npc, 0);
		else
			CUR_SCHED(npc).state = 0;
		break;
	}
}
