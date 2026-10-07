/* Serpent Isle SI.EXE, overlay segment 277 (file offsets 0x077f70 to 0x078503, 1427 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include "activity.h"
#include "item.h"
#include "npcref.h"
#include "u7npc.h"
#include "sprite.h"
#include "random.h"
#include "actitem.h"
#include "actutil.h"
#include "combatai.h"
#include "text.h"
#include "script.h"
#include "mapview.h"
#include "search.h"

#define NPC(p) GetNpcBufferForIbo(p)
#define CUR_SCHED(p) (NPC(p)->schedules[NPC(p)->currentSchedule])

/* Preach: at the podium, exhort the congregation or visit one of its members. */
void RunPreachSchedule(objref *npc)
{
	objref member;
	AreaSearch unusedSearch1, unusedSearch2;

	switch (CUR_SCHED(npc).state) {
	case 0:
		Npc_pushSchedule(npc, WORK_GOTO_SCH_D, -1, -1, 0);
		break;
	case 1:
		NPC(npc)->scheduleToggle = 1;
		CUR_SCHED(npc).state++;
		break;
	case 2:
		if (RollChance(3)) {
			CUR_SCHED(npc).state = 1;
			Npc_pushSchedule(npc, WORK_CHECK_AREA, 1, -1, 0);
			break;
		}
		if (NPC(npc)->scheduleToggle)
			CUR_SCHED(npc).state = 10;
		else
			CUR_SCHED(npc).state = (GenerateRandomIntegerInRange(3) + 1) * 10;
		NPC(npc)->scheduleToggle = !NPC(npc)->scheduleToggle;
		break;
	case 10:
		Npc_pushSchedule(npc, WORK_GOTO_ITEM, 697 /* podium */, -1, 0);
		break;
	case 11:
		if (IsBlocked(npc)) {
			Npc_setSchedule(npc, WORK_LOITER);
			CUR_SCHED(npc).state = 1;
		} else
			CUR_SCHED(npc).state++;
		break;
	case 12:
		PostScheduleScript(npc, MakeScript(SCRIPT_WAIT, 20, SCRIPT_END));
		break;
	case 13:
		/* "Strive for unity!", "Trust thy brother!" and three more */
		SpriteManager_barkOnItem(&gSpriteManager, *npc, GetGameText(1, GenerateRandomIntegerInRange(5) + 3), 0, 15, 0);
		PostScheduleScript(npc, MakeScript(SCRIPT_WAIT, 20, SCRIPT_END));
		member = ChooseNpcInFront(npc, 0);
		/* "I am worthy!", "I believe!", "Yes!" or "Yea, verily!" */
		if (member.valid())
			SpriteManager_barkOnItem(&gSpriteManager, member,
				GetGameText(1, GenerateRandomIntegerInRange(4) + 12), 0, 15, 0);
		break;
	case 14:
		CUR_SCHED(npc).state = 2;
		break;
	case 20:
		member = ChooseNpcInFront(npc, 1);
		if (member.valid()) {
			NPC(npc)->scheduleValue = Item_getNpcNumber(&member);
			CUR_SCHED(npc).state++;
		} else
			CUR_SCHED(npc).state = 2;
		break;
	case 21:
		GetNpcIbo(&member, NPC(npc)->scheduleValue);
		if (member.valid() && CanVisit(&member)) {
			if (!PathNextToItem(npc, member, 100))
				CUR_SCHED(npc).state = 1;
			else if (Item_greatestDeltaToItem(*npc, member) <= 1)
				CUR_SCHED(npc).state++;
		} else
			CUR_SCHED(npc).state = 2;
		break;
	case 22:
		/* "Say it with me, brother!", "Art thou with us, brother?" and two more */
		SpriteManager_barkOnItem(&gSpriteManager, *npc, GetGameText(1, GenerateRandomIntegerInRange(4) + 8), 0, 15, 0);
		PostScheduleScript(npc, MakeScript(SCRIPT_STAND_FRAME, SCRIPT_WAIT, 30, SCRIPT_END));
		break;
	case 23:
		CUR_SCHED(npc).state = 2;
		break;
	case 30:
	case 31:
		CUR_SCHED(npc).state = 2;
		break;
	}
}
