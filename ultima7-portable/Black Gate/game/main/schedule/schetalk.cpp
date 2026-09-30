/* Black Gate U7.EXE, overlay segment 274 (file offsets 0x080f80 to 0x081234, 692 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder inferred from compiler flags and link order.
 */

#include "u7port.h"
#include "activity.h"
#include "lowlevel.h"
#include "u7npc.h"
#include "item.h"
#include "npcref.h"
#include "voolook.h"
#include "makemojo.h"
#include "sprite.h"
#include "combatai.h"
#include "missile.h"
#include "monsters.h"
#include "random.h"
#include "actitem.h"
#include "text.h"
#include "usehook.h"
#include "script.h"

#define NPC(p) GetNpcBufferForIbo(p)
#define CUR_SCHED(p) (NPC(p)->schedules[NPC(p)->currentSchedule])
#define FLAG(v, m) ((uint8_t) ((v) & (m)))

/*
 * Talk: walk up to the avatar, calling out now and then, and start a conversation once close and in
 * sight. An NPC that cannot see the invisible avatar, or cannot reach him, loiters instead.
 */
void RunTalkSchedule(objref *npc)
{
	int16_t monster;
	uint16_t type;

	switch (CUR_SCHED(npc).state) {
	case 0:
		Npc_pushSchedule(npc, WORK_GOTO_SCH_D, -1, -1);
		break;
	case 1:
		type = objref(npc->off).type();
		monster = MonsterLookup.get(type);
		/* quality flag bit 0: the avatar is invisible */
		if (FLAG(Item_getQualityFlags(&AvatarRef), QUALITY_INVISIBLE)
			&& !(uint8_t) MonsterRecords.get(monster)->seeInvisible
			|| !PathNextToItem(npc, AvatarRef, 75)) {
			CUR_SCHED(npc).state = 0;
			Npc_pushSchedule(npc, WORK_LOITER, -1, -1);
			CUR_SCHED(npc).state = 1;
		} else if (Item_greatestDeltaToItem(*npc, AvatarRef) <= 3 && HasLineOfFire(*npc, AvatarRef))
			CUR_SCHED(npc).state++;
		else if (RollChance(20))
			/* "Stop!", "I would have words with thee." or "I wish to speak to thee." */
			SpriteManager_barkOnItem(&gSpriteManager, *npc,
				GetGameText(1, GenerateRandomIntegerInRange(3) + 20), 0, 15, 0);
		break;
	case 2:
		PostScheduleScript(npc, MakeScript(SCRIPT_STAND_FRAME, SCRIPT_END));
		break;
	case 3:
		RunUsable(1, *npc, -1);
		break;
	}
}
