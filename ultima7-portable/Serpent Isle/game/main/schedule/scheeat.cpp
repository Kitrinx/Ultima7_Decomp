/* Serpent Isle SI.EXE, overlay segment 275 (file offsets 0x0777a0 to 0x077a5b, 699 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include "activity.h"
#include "item.h"
#include "coord.h"
#include "u7npc.h"
#include "sprite.h"
#include "sortitem.h"
#include "random.h"
#include "text.h"
#include "search.h"

#define NPC(p) GetNpcBufferForIbo(p)
#define CUR_SCHED(p) (NPC(p)->schedules[NPC(p)->currentSchedule])

/* Eat at the inn: sit, then now and then eat the food ahead or call for service. */
void RunEatAtInnSchedule(objref *npc)
{
	AreaSearch food;
	int16_t x, y;
	int16_t dir;

	switch (CUR_SCHED(npc).state) {
	case 0:
		Npc_pushSchedule(npc, WORK_GOTO_SCH_D, -1, -1, 0);
		break;
	case 1:
		Npc_pushSchedule(npc, WORK_SIT, -1, -1, 0);
		break;
	case 2:
		if (IsBlocked(npc)) {
			Npc_setSchedule(npc, WORK_LOITER);
			CUR_SCHED(npc).state = 1;
		} else
			Npc_pushSchedule(npc, WORK_PAUSE, GenerateRandomIntegerInRange(61) + 40, -1, 0);
		break;
	case 3:
		x = Item_getX(*npc);
		y = Item_getY(*npc);
		dir = (uint8_t) (NPC(npc)->status & 7);
		if (RollChance(2)) {
			FindItemInArea(&food, Coord(x + DirDeltaX[dir]), Coord(y + DirDeltaY[dir]),
				Coord(x + DirDeltaX[dir]), Coord(y + DirDeltaY[dir]),
				0, 377 /* food item */, 0xff, 0xff);
			if (food.found()) {
				Item_delete(&food.current);
				/* "Mmmm, tasty!", "Burp!" or "Mmmm..." */
				SpriteManager_barkOnItem(&gSpriteManager, *npc,
					GetGameText(1, GenerateRandomIntegerInRange(3) + 37), 0, 15, 0);
			}
		} else
			/* "More food!", "Service!", "Barkeeper!", "Ale!" or "Menu!" */
			SpriteManager_barkOnItem(&gSpriteManager, *npc,
				GetGameText(1, GenerateRandomIntegerInRange(5) + 32), 0, 15, 0);
		CUR_SCHED(npc).state = 2;
		break;
	}
}
