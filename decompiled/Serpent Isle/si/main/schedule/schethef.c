/* Serpent Isle SI.EXE, overlay segment 276 (file offsets 0x077a90 to 0x077f16, 1158 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "activity.h"
#include "iteminfo.h"
#include "u7npc.h"
#include "item.h"
#include "npcref.h"
#include "coord.h"
#include "sprite.h"
#include "combatai.h"
#include "random.h"
#include "actitem.h"
#include "missile.h"
#include "search.h"
#include "text.h"
#include "npcpath.h"
#include "script.h"

extern int DiscardedPathLength[2];

#define NPC(p) GetNpcBufferForIbo(p)
#define CUR_SCHED(p) (NPC(p)->schedules[NPC(p)->currentSchedule])

/* Thief: follow the avatar, and when close enough pick his pack for gold. */
void far RunThiefSchedule(objref *npc)
{
	AreaSearch pack;

	switch (CUR_SCHED(npc).state) {
	case 0:
		Npc_pushSchedule(npc, WORK_GOTO_SCH_D, -1, -1, 0);
		break;
	case 1:
		if (Item_greatestDeltaToItem(*npc, AvatarRef) > 8) {
			if (!PathNextToItem(npc, AvatarRef, 100))
				CUR_SCHED(npc).state = 3;
		} else if (RollChance(5))
			CUR_SCHED(npc).state++;
		else
			CUR_SCHED(npc).state = 3;
		break;
	case 2:
		if (PathNextToItem(npc, AvatarRef, 100)) {
			if (Item_greatestDeltaToItem(*npc, AvatarRef) <= 2 && HasLineOfFire(*npc, AvatarRef)) {
				/* "Nice weather today.", "Good day.", "How art thou doing?" or "Greetings." */
				SpriteManager_barkOnItem(&gSpriteManager, *npc,
					GetGameText(1, GenerateRandomIntegerInRange(4) + 16), 0, 15, 0);
				PostScheduleScript(npc, MakeScript(SCRIPT_WAIT, 15, SCRIPT_END));
				FindItemInContainer(&pack, AvatarRef, 0, -1, 0xff, 0xff);
				while (pack.found()) {
					if (IsTemporary(&pack.current) || pack.current.type() == 644 /* gold coin */
						|| pack.current.type() == 645 /* gold nugget */ || pack.current.type() == 646 /* gold bar */) {
						Item_moveIntoContainer(&pack.current, *npc);
						break;
					}
					FindItem(&pack);
				}
			}
		} else
			CUR_SCHED(npc).state = 1;
		break;
	case 3:
		NPC(npc)->scheduleValue = 0;
		CUR_SCHED(npc).state++;
		break;
	case 4:
		if (StartPath(*npc,
			Coord(GenerateRandomIntegerInRange(17) + Item_getX(AvatarRef).value - 8),
			Coord(GenerateRandomIntegerInRange(17) + Item_getY(AvatarRef).value - 8),
			Item_getZ(&AvatarRef), 100, DiscardedPathLength, 0) == 0)
			CUR_SCHED(npc).state++;
		else if (NPC(npc)->scheduleValue > 3)
			CUR_SCHED(npc).state = 1;
		else
			NPC(npc)->scheduleValue++;
		break;
	case 5:
		ContinueScheduleWalk(npc);
		if (Item_greatestDeltaToItem(*npc, AvatarRef) > 8) {
			StopPaths(*npc);
			CUR_SCHED(npc).state = 1;
		}
		break;
	case 6:
		PostScheduleScript(npc, MakeScript(SCRIPT_WAIT, GenerateRandomIntegerInRange(6) + 5, SCRIPT_END));
		CUR_SCHED(npc).state = 1;
		break;
	}
}
