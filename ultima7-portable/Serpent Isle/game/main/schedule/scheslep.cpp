/* Serpent Isle SI.EXE, overlay segment 266 (file offsets 0x071cb0 to 0x073041, 5009 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include "activity.h"
#include "lowlevel.h"
#include "item.h"
#include "coord.h"
#include "u7npc.h"
#include "random.h"
#include "npcpath.h"
#include "voolook.h"
#include "makemojo.h"
#include "sprite.h"
#include "party.h"
#include "actitem.h"
#include "actqueue.h"
#include "monsters.h"
#include "usehook.h"
#include "script.h"
#include "actutil.h"
#include "missile.h"
#include "search.h"
#include "text.h"
#include "camera.h"
#include "type.h"
#include "combatai.h"

/* the two kinds of bed */
#define TYPE_BED        1011
#define TYPE_BED_2      696

extern objref AvatarRef;
extern int16_t DiscardedPathLength[2];
/* the bed usecode sends the avatar to */
extern int16_t NapBed;

#define NPC(p) GetNpcBufferForIbo(p)
#define CUR_SCHED(p) (NPC(p)->schedules[NPC(p)->currentSchedule])
/* the type of bed the sleeper has chosen */
#define BED_TYPE(p) NPC(p)->scheduleValue

inline uint8_t IsAvatar(objref *p)
{
	return Item_isAvatar(p);
}

/* quality flag bit 0 */
inline uint8_t IsInvisible(objref *p)
{
	return Item_getQualityFlags(p) & QUALITY_INVISIBLE;
}

inline uint8_t SeesInvisible(int16_t monster)
{
	return MonsterRecords.get(monster)->seeInvisible;
}

/*
 * Sleep: find the nearest bed, walk to it and lie down; anyone too big for a bed sleeps on the ground.
 * A sleeper may wake to chase an avatar who comes close, and some kinds of creature cry out or attack.
 * The avatar instead runs the bed's usecode.
 */
void RunSleepSchedule(objref *npc)
{
	Coord x, y;
	int8_t result;
	int16_t z, floorZ;
	uint8_t dir;
	objref chosen;
	int8_t fitsBed;
	uint16_t type;
	int16_t monster;
	int16_t i;
	AreaSearch nearA, nearB, bestA, bestB, blocker, bedTop, bed;

	switch (CUR_SCHED(npc).state) {
	case 0:
		Npc_pushSchedule(npc, WORK_GOTO_SCH_D, -1, -1, 0);
		break;
	case 1:
		if (IsAvatar(npc))
			CUR_SCHED(npc).state++;
		else
			Npc_pushSchedule(npc, WORK_CHECK_AREA, 1, -1, 0);
		break;
	case 2:
		if (IsBlocked(npc)) {
			if (IsAvatar(npc))
				CUR_SCHED(npc).state = 20;
			else
				CUR_SCHED(npc).state++;
		} else
			CUR_SCHED(npc).state = 1;
		break;
	case 3:
		/* pick the nearer of the two bed types and walk to it */
		x = Item_getX(*npc);
		y = Item_getY(*npc);
		z = GetItemZAndStuff(npc).z();
		/* only a sleeper one cell across fits a bed; the bed's frames are searched one at a time */
		if (GetFootprintX(ITEM(npc->off)->asTypeFrame()) == 0 && GetFootprintY(ITEM(npc->off)->asTypeFrame()) == 0)
			fitsBed = 1;
		else
			fitsBed = 0;
		for (i = 0; i < 7; i++) {
			if (fitsBed) {
				FindNearestItem(&nearA, x, y, 25, 0, TYPE_BED, 0xff, i);
				FindNearestItem(&nearB, x, y, 25, 0, TYPE_BED_2, 0xff, i);
			}
			if (!bestA.found() || GetDistance(x, y, z, Item_getX(nearA.current),
					Item_getY(nearA.current), GetItemZAndStuff(&nearA.current).z()) <
					GetDistance(x, y, z, Item_getX(bestA.current), Item_getY(bestA.current),
					GetItemZAndStuff(&bestA.current).z()))
				bestA = nearA;
			if (!bestB.found() || GetDistance(x, y, z, Item_getX(nearB.current),
					Item_getY(nearB.current), GetItemZAndStuff(&nearB.current).z()) <
					GetDistance(x, y, z, Item_getX(bestB.current), Item_getY(bestB.current),
					GetItemZAndStuff(&bestB.current).z()))
				bestB = nearB;
		}
		if (bestA.found() && !bestB.found())
			BED_TYPE(npc) = TYPE_BED;
		else if (!bestA.found() && bestB.found())
			BED_TYPE(npc) = TYPE_BED_2;
		else if (!bestA.found() && !bestB.found()) {
			CUR_SCHED(npc).state = 10;
			break;
		} else {
			if (Item_greatestDeltaToItem(*npc, bestA.current) <=
					Item_greatestDeltaToItem(*npc, bestB.current))
				BED_TYPE(npc) = TYPE_BED;
			else
				BED_TYPE(npc) = TYPE_BED_2;
		}
		chosen = BED_TYPE(npc) == TYPE_BED ? bestA.current.off : bestB.current.off;
		if (UseSpotFinder.findUseSpot(npc, chosen, &x.value, &y.value, &z, &dir, -1)) {
			result = StartPath(*npc, x, y, z, 100, DiscardedPathLength, 0);
			NPC(npc)->direction = dir;
			if (result == 0) {
				CUR_SCHED(npc).state++;
				break;
			}
		} else {
			/* nowhere to sleep: the avatar rejoins the party, anyone else loiters */
			if (IsAvatar(npc))
				SetPartyWorkType(WORK_FOLLOW_AVT);
			else {
				Npc_setSchedule(npc, WORK_LOITER);
				CUR_SCHED(npc).state = 1;
			}
			break;
		}
		if (result == 2) {
			if (IsAvatar(npc))
				SetPartyWorkType(WORK_FOLLOW_AVT);
			else {
				Npc_setSchedule(npc, WORK_LOITER);
				CUR_SCHED(npc).state = 1;
			}
		}
		break;
	case 4:
		ContinueScheduleWalk(npc);
		break;
	case 5:
		PostScheduleScript(npc, MakeScript(SCRIPT_FACE, NPC(npc)->direction + 48, SCRIPT_END));
		break;
	case 6:
		/* arrived: climb into the bed unless something lies on it */
		if (GetFootprintX(ITEM(npc->off)->asTypeFrame()) == 0 && GetFootprintY(ITEM(npc->off)->asTypeFrame()) == 0)
			fitsBed = 1;
		else
			fitsBed = 0;
		floorZ = z = GetItemZAndStuff(npc).z();
		if (fitsBed)
			z++;
		x = Item_getX(*npc);
		y = Item_getY(*npc);
		if (fitsBed)
			FindNearestItemInZRange(&bedTop, x, y, 5, 0, BED_TYPE(npc), 0xff, 0xff, z, z);
		FindNearestItemInZRange(&bed, x, y, 5, 0, BED_TYPE(npc), 0xff, 0xff, floorZ, floorZ);
		x = Item_getX(bed.current);
		y = Item_getY(bed.current);
		FindItemInArea(&blocker, x, y, x, y, 4, -1, 0xff, 0xff, z, z);
		if (blocker.found() || !bed.found() || IsBlocked(npc)) {
			if (IsAvatar(npc))
				SetPartyWorkType(WORK_FOLLOW_AVT);
			else {
				Npc_setSchedule(npc, WORK_LOITER);
				CUR_SCHED(npc).state = 1;
			}
			break;
		}
		if (fitsBed && bedTop.found() && bedTop.current.frame() & 1)
			Item_setFrame(&bedTop.current, bedTop.current.frame() + 1);
		if (bed.current.type() == TYPE_BED && bed.current.frame() == 17)
			Item_move(npc, x + 1, y, z);
		else
			Item_move(npc, x, y, z);
		/* the facing bits: lie along the bed */
		if (BED_TYPE(npc) == TYPE_BED)
			NPC(npc)->changeFlags(7, 0);
		else
			NPC(npc)->changeFlags(7, 6);
		ActionQueue.run(ActionQueue.add(*npc, MakeScript(SCRIPT_LIE_FRAME, SCRIPT_END)));
		if (IsAvatar(npc)) {
			DrawWorld(&gCamera);
			RunUsable(1, bed.current, 0x622);
			/* 0x80 is the asleep bit */
			NPC(npc)->changeFlags(0x80, 0);
			Npc_setSchedule(npc, WORK_FOLLOW_AVT);
			CUR_SCHED(npc).state = 1;
		} else
			NPC(npc)->changeFlags(0, 0x80);
		CUR_SCHED(npc).state++;
		break;
	case 7:
		/* asleep: now and then, the brighter the more often, wake to chase a visible avatar nearby */
		if (GenerateRandomIntegerInRange(750) < Npc_getIntelligence(npc) && !IsAvatar(npc) &&
				Item_greatestDeltaToItem(*npc, AvatarRef) <= 15 && HasLineOfFire(*npc, AvatarRef)) {
			monster = GetMonsterNumber(*npc);
			if (!IsInvisible(&AvatarRef) || SeesInvisible(monster)) {
				type = npc->type();
				Npc_setSchedule(npc, WORK_SLEEP);
				if ((uint16_t) Item_getNpcNumber(npc) < 256)
					CUR_SCHED(npc).state = 255;
				WakeUpNpc(npc);
				if (!(MonsterRecords.get(monster)->extraFlags & 0x20)) {
					switch (npc->type()) {
					case 691 /* goblin */:
					case 354 /* Goblin King */:
					case 478 /* goblin */:
					case 725 /* goblin */:
					case 744 /* goblin */:
						/* "Traf lagar!" */
						SpriteManager_barkOnItem(&gSpriteManager, *npc, GetGameText(1, 202), 3, 15, 0);
						break;
					case 214 /* gwani */:
					case 862 /* gwani */:
					case 867 /* gwani child */:
						/* "Grrrr..." or "Gaarrrrr!" */
						SpriteManager_barkOnItem(&gSpriteManager, *npc,
							GetGameText(1, GenerateRandomIntegerInRange(2) + 234), 3, 15, 0);
						if (GetAlignment(npc) == 2) {
							Npc_pushSchedule(npc, WORK_COMBAT, -1, -1, 0);
							CUR_SCHED(npc).state = 1;
							Npc_setTarget((NPCRef *) npc, Item_getNpcNumber(&AvatarRef), 1);
						}
						break;
					case 392 /* Xenka */:
					case 250 /* monk */:
						/* NPCs 208, 210 and 214 wake without a word */
						if (Item_getNpcNumber(npc) == 208 || Item_getNpcNumber(npc) == 210 ||
								Item_getNpcNumber(npc) == 214)
							break;
						SpriteManager_barkOnItem(&gSpriteManager, *npc,
							GetGameText(1, GenerateRandomIntegerInRange(6) + 149), 3, 15, 0);
						break;
					case 480 /* gargoyle */:
					case 883 /* wingless Gargoyle */:
						/* turn hostile and attack: "To attack!" and two more */
						NPC(npc)->setAlignment(2);
						Npc_pushSchedule(npc, WORK_COMBAT, -1, -1, 0);
						CUR_SCHED(npc).state = 1;
						Npc_setTarget((NPCRef *) npc, Item_getNpcNumber(&AvatarRef), 1);
						SpriteManager_barkOnItem(&gSpriteManager, *npc,
							GetGameText(1, GenerateRandomIntegerInRange(3) + 231), 3, 15, 0);
						return;
					default:
						/* "What was that noise?", "Who goes there?" and four more */
						SpriteManager_barkOnItem(&gSpriteManager, *npc,
							GetGameText(1, GenerateRandomIntegerInRange(6) + 149), 3, 15, 0);
					}
				}
				Npc_pushSchedule(npc, WORK_HOUND, -1, -1, 0);
				CUR_SCHED(npc).state = 1;
			}
		}
		break;
	case 10:
		/* no bed: sleep on the ground */
		PostScheduleScript(npc, MakeScript(SCRIPT_LIE_FRAME, SCRIPT_END));
		if (!IsAvatar(npc))
			NPC(npc)->changeFlags(0, 0x80);
		break;
	case 20:
		/* the avatar walks to the bed usecode chose */
		if (UseSpotFinder.findUseSpot(npc, NapBed, &x.value, &y.value, &z, &dir, -1)) {
			result = StartPath(*npc, x, y, z, 100, DiscardedPathLength, 0);
			NPC(npc)->direction = dir;
		} else {
			SetPartyWorkType(WORK_FOLLOW_AVT);
			break;
		}
		if (result == 0) {
			BED_TYPE(npc) = objref(NapBed).type();
			CUR_SCHED(npc).state++;
		} else if (result == 2)
			SetPartyWorkType(WORK_FOLLOW_AVT);
		break;
	case 21:
		CUR_SCHED(npc).state = 4;
		break;
	}
}
