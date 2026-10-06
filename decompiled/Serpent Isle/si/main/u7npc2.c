/* Serpent Isle SI.EXE, resident segment 38 (file offsets 0x01c57b to 0x01d2f6, 3451 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include <dos.h>
#include "lowlevel.h"
#include "activity.h"
#include "dosio.h"
#include "item.h"
#include "voolook.h"
#include "easyfile.h"
#include "u7manage.h"
#include "datanode.h"
#include "sche.h"
#include "itembuf.h"
#include "makemojo.h"
#include "monsters.h"
#include "wihh.h"
#include "sprite.h"
#include "combat.h"
#include "combatai.h"
#include "eggspawn.h"
#include "party.h"
#include "u7sound.h"
#include "random.h"
#include "actqueue.h"
#include "npcpath.h"
#include "text.h"
#include "npcref.h"
#include "mapview.h"
#include "crime.h"
#include "u7npc.h"
#include "equip.h"
#include "voonpc.h"
#include "actitem.h"
#include "worldpal.h"

#define NPC(p) GetNpcBufferForIbo(p)
#define CURRENT(p) (NPC(p)->schedules[NPC(p)->currentSchedule])
#define NPC_FLAG(r, mask) ((unsigned char)((NPC(r)->status & (mask)) != 0))
#define TYPE(rec) ((rec)->typeFrame & 0x3ff)
#define VISIBLE(r) ((char)(GetItemKind(r) == LOCATION_OFF_MAP) ? 0 : IsItemInCellWindow((r)->off))

/* IsInCombat on a copy of the reference. */
inline unsigned char IsFighting(NPCRef r)
{
	NPCRef *p = &r;
	return NPC(p)->workType == WORK_COMBAT && IsOnOutermostSchedule(p);
}

/* The U7NBUF.DAT file in a saved game: every NPC buffer and equip list. */
struct NpcSaver : DataNode {
	int unused;
	char *name();
	void load(char *dir);
	void save(char *dir);
	void refresh(char *dir);
};

extern int NpcItemRefs[];

char *U7NBufFileName = "U7NBUF.DAT";
NpcSaver NpcBufferFile;

void Npc_runSchedule(objref *who)
{
	if (CURRENT(who).kind <= 0)
		return;
	if (CURRENT(who).kind >= 53)
		return;
	if (WorkTypeHandlers[CURRENT(who).kind] == 0)
		return;

	int x = Item_getX(*who);
	int y = Item_getY(*who);
	int type = TYPE(ITEM(who->off));
	if ((type == 744 || type == 354 || type == 691 || type == 478 || type == 725) &&
		!IsNpcUnconscious(who) && x > 576 && x < 832 && y > 1200 && y < 1536) {
		if (!GameScreen.isDark()) {
			if (Item_greatestDeltaToItem(*who, AvatarRef) < 20) {
				Npc_setSchedule(who, WORK_COMBAT);
				CURRENT(who).state = 1;
				Npc_setTarget(&NPCRef(*who), Item_getNpcNumber(&AvatarRef), 1);
				SpriteManager_barkOnItem(&gSpriteManager, who->off,
					GetGameText(1, GenerateRandomIntegerInRange(2) + 198), 5, 15, 0);
				CombatGroups.callGuards(1);
				return;
			}
		} else {
			if (Item_greatestDeltaToItem(*who, AvatarRef) < 5) {
				Npc_setSchedule(who, WORK_COMBAT);
				CURRENT(who).state = 1;
				Npc_setTarget(&NPCRef(*who), Item_getNpcNumber(&AvatarRef), 1);
				SpriteManager_barkOnItem(&gSpriteManager, who->off,
					GetGameText(1, 198), 5, 15, 0);
				CombatGroups.callGuards(1);
				return;
			}
		}
	}
	objref ref;
	ref = objref(who->off);
	(*WorkTypeHandlers[CURRENT(who).kind])(&ref);
}

void Npc_clearTargets(objref *who)
{
	GetNpcBufferForIbo(who)->primaryTarget = -1;
	GetNpcBufferForIbo(who)->secondaryTarget = -1;
	GetNpcBufferForIbo(who)->oppressor = -1;
}

/* Starts a new work type: resets the schedule stack, picks the attack mode (party members keep
 * theirs), and on entering combat readies a weapon, may bark a battle cry and starts battle music. */
void Npc_setSchedule(objref *who, char activity)
{
	int i, type, grade;
	objref other;
	unsigned char bark;

	if ((unsigned char)Item_isAvatar(who)) {
		if (activity == WORK_FOLLOW_AVT)
			ResetPartyFormation();
		AvatarInCombat = activity == WORK_COMBAT;
		if (NPC(who)->workType == WORK_COMBAT && activity == WORK_FOLLOW_AVT && CombatGroups.countEnemies())
			PlayMusic(7);
	}
	NPC(who)->iVr[0] = -1;
	NPC(who)->typeFlagsHigh = NPC(who)->typeFlagsHigh & ~0x20;
	NPC(who)->currentSchedule = 0;
	CURRENT(who).state = 0;
	CURRENT(who).kind = activity;
	CURRENT(who).x = 0;
	CURRENT(who).y = 0;
	CURRENT(who).counter = 0;
	NPC(who)->scheduleValue = 0;
	NPC(who)->scheduleToggle = 0;
	NPC(who)->typeFlagsHigh = NPC(who)->typeFlagsHigh | 1;
	if (NPC_FLAG(who, NPC_IN_PARTY) || (unsigned char)(NPC(who)->typeFlagsHigh & 0x40)) {
		Item_setQuality(who, NPC(who)->defaultAttackMode);
	} else {
		grade = 0;
		type = MonsterLookup.get(TYPE(ITEM(who->off)));
		for (; grade < 3; grade++) {
			if (!GenerateRandomIntegerInRange(
				CategoryAttackOdds[(unsigned char)(MonsterRecords.get(type)->category - 1)][grade]))
				break;
		}
		Item_setQuality(who,
			CategoryAttackModes[(unsigned char)(MonsterRecords.get(type)->category - 1)][grade]);
		NPC(who)->defaultAttackMode =
			CategoryAttackModes[(unsigned char)(MonsterRecords.get(type)->category - 1)][grade];
	}
	Npc_clearTargets(who);
	if (activity == WORK_COMBAT) {
		NPC(who)->typeFlagsHigh = NPC(who)->typeFlagsHigh & ~4;
		NPC(who)->face = 0;
		SelectWeapon(*who, 0, 0);
		if (!(unsigned char)Item_isAvatar(who) && RollChance(3)) {
			if (NPC_FLAG(who, NPC_IN_PARTY) || (unsigned char)(NPC(who)->typeFlagsHigh & 0x40)) {
				bark = 0;
				for (i = 0; i < NPC_COUNT; i++) {
					GetNpcIbo(&other, i);
					if (other.valid() && VISIBLE(&other) &&
						!NPC_FLAG(&other, NPC_IN_PARTY) && !(unsigned char)(NPC(&other)->typeFlagsHigh & 0x40) &&
						NPC(&other)->workType == WORK_COMBAT) {
						bark = 1;
						break;
					}
				}
			} else
				bark = 1;
			int monster = MonsterLookup.get(GetItemType(*who));
		if (bark && !(unsigned char)(MonsterRecords.get(monster)->extraFlags & 0x20)) {
			switch (TYPE(ITEM(who->off))) {
			case 354:
			case 478:
			case 691:
			case 725:
			case 744:
				SpriteManager_barkOnItem(&gSpriteManager, who->off, GetGameText(1, 218), 3, 15, 0);
				break;
			case 298:
				SpriteManager_barkOnItem(&gSpriteManager, who->off, GetGameText(1, 230), 3, 15, 0);
				break;
			default:
				SpriteManager_barkOnItem(&gSpriteManager, who->off,
					GetGameText(1, GenerateRandomIntegerInRange(3) + 57), 3, 15, 0);
				break;
			}
		}
		}
		ActionQueue.remove(who->off, 1, 0);
		if (NPC(who)->workType == WORK_THIEF)
			NPC(who)->setAlignment(2);
		if (!NPC_FLAG(who, NPC_DEAD) && VISIBLE(who) &&
			(GetAlignment(who) == 3 || GetAlignment(who) == 2)) {
			if (!(int)SpecialMusicPlaying)
				PlayMusic(1);
			CombatGroups.battleMusic = 1;
		}
	}
	StopPaths(*who);
	NPC(who)->workType = activity;
}

void Npc_pushSchedule(objref *who, char activity, int x, int y, char counter)
{
	if (NPC(who)->currentSchedule < 5) {
		NPC(who)->currentSchedule++;
		CURRENT(who).kind = activity;
		CURRENT(who).state = 0;
		CURRENT(who).x = x;
		CURRENT(who).y = y;
		CURRENT(who).counter = counter;
	}
}

void Npc_popSchedule(objref *who, int result)
{
	NPC(who)->result = result;
	if (NPC(who)->currentSchedule > 0) {
		NPC(who)->currentSchedule--;
		if (!IsFighting(*who))
			CURRENT(who).state++;
	}
}
