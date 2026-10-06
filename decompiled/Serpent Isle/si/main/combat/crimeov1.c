/* Serpent Isle SI.EXE, overlay segment 355 (file offsets 0x0ae9b0 to 0x0b01d2, 6178 bytes).
 * Borland C++ 2.0 -mm -O -G -P -Y rebuilds it byte for byte as C++.
 */

#include "lowlevel.h"
#include "activity.h"
#include "iteminfo.h"
#include "item.h"
#include "npcref.h"
#include "coord.h"
#include "voolook.h"
#include "easyfile.h"
#include "chkfile.h"
#include "u7npc.h"
#include "collide.h"
#include "partymov.h"
#include "datanode.h"
#include "makemojo.h"
#include "sprite.h"
#include "combat.h"
#include "actitem.h"
#include "actqueue.h"
#include "actutil.h"
#include "combatai.h"
#include "combmode.h"
#include "missile.h"
#include "monsters.h"
#include "equip.h"
#include "sortitem.h"
#include "text.h"
#include "u7sound.h"
#include "usehook.h"
#include "random.h"
#include "script.h"
#include "legalmov.h"
#include "type.h"
#include "attack.h"
#include "u7ibuf.h"
#include "mapview.h"
#include "crimeov1.h"

unsigned char CrimeUsecodeOff = 0;     /* never referenced */

struct MonsterRef {
	int index;
	MonsterRef() {}
	MonsterRef(int n) { index = n; }
	MonsterRecord *operator->() { return MonsterRecords.get(index); }
	unsigned char none() { return index == 0; }
};

#define NPC(p) GetNpcBufferForIbo(p)
#define CUR_SCHED(p) NPC(p)->schedules[NPC(p)->currentSchedule]
#define NPC_TYPE(n) (ITEM((n).off)->typeFrame & 0x3ff)

inline unsigned char IsAsleep(objref *p) { return (NPC(p)->status & NPC_ASLEEP) != 0; }
inline unsigned char IsParalyzed(objref *p) { return (NPC(p)->status & NPC_PARALYZED) != 0; }
inline unsigned char HasNoHealth(objref *p) { return (char)Item_getHitPoints(p) <= 0; }
inline unsigned char IsKindAtLeast(ItemInfo far &info, unsigned char value)
{
	return info.kind() >= value;
}

int Combat::countHostileGuards()
{
	NPCRef npc;
	int i, count;
	count = 0;
	for (i = 0; i < NPC_COUNT; i++) {
		GetNpcIbo(&npc, i);
		if (npc.valid() && Item_greatestDeltaToItem(npc, AvatarRef) < 31 &&
			CUR_SCHED(&npc).kind == WORK_ARREST && GetAlignment(&npc) == 3)
			count++;
	}
	return count;
}

void far ReportCrime(int thing, char hostile, char runUsable)
{
	NPCRef npc;
	unsigned char noticed, alerted, woken;
	MonsterRef monster;
	objref owner;
	int i;

	noticed = 0;
	alerted = 0;
	if (InDungeon)
		return;
	for (owner = objref(thing); IsKindAtLeast(GetItemZAndStuff(&owner), 6);
		owner = Item_getContainer(&owner))
		;
	for (i = 0; i < NPC_COUNT; i++) {
		GetNpcIbo(&npc, i);
		if (!(npc.valid() && CanVisit(&npc) &&
			(Item_greatestDeltaToItem(npc, AvatarRef) <= 20 ||
			 owner.valid() && Item_greatestDeltaToItem(npc, owner) <= 20) &&
			!IsInParty(&npc) && !IsInMode(&npc, ATTACK_FLEE) && IsSentient(npc) &&
			GetAlignment(&npc) == 0 && !(unsigned char)(NPC(&npc)->typeFlagsHigh & 0x40) &&
			NPC(&npc)->workType != WORK_WAIT))
			continue;
		if (IsParalyzed(&npc) || HasNoHealth(&npc) || IsDead(&npc))
			continue;
		if (IsAsleep(&npc)) {
			if (NPC(&npc)->workType == WORK_SLEEP && RollChance(3))
				woken = 1;
			else
				continue;
		} else
			woken = 0;
		if (!(HasLineOfFire(AvatarRef, npc) || owner.valid() && HasLineOfFire(owner, npc)))
			continue;
		noticed = 1;
		if (woken)
			WakeUpNpc(&npc);
		if ((CUR_SCHED(&npc).kind == WORK_HOUND && RollChance(3)) || NPC(&npc)->workType == WORK_COMBAT) {
			alerted = 1;
			hostile = 1;
		}
		monster.index = GetMonsterNumber(npc);
		if ((unsigned char)(Item_getQualityFlags(&AvatarRef) & QUALITY_INVISIBLE) &&
			!(unsigned char)monster->seeInvisible) {
			switch (NPC_TYPE(npc)) {
			case 478: case 354: case 691: case 725: case 744:
				SpriteManager_barkOnItem(&gSpriteManager, npc,
					GetGameText(1, 200), 3, 15, 0);
				break;
			case 250: case 392:
				if (Item_getNpcNumber(&npc) != 208 && Item_getNpcNumber(&npc) != 210 && Item_getNpcNumber(&npc) != 214)
					SpriteManager_barkOnItem(&gSpriteManager, npc,
						GetGameText(1, 133), 3, 15, 0);
				break;
			case 298:
				SpriteManager_barkOnItem(&gSpriteManager, npc,
					GetGameText(1, 227), 3, 15, 0);
				break;
			default:
				if (!(monster->extraFlags & 0x20))
					SpriteManager_barkOnItem(&gSpriteManager, npc,
						GetGameText(1, GenerateRandomIntegerInRange(3) + 133), 3, 15, 0);
			}
			continue;
		}
		if (hostile && !woken) {
			switch (NPC_TYPE(npc)) {
			case 478: case 354: case 691: case 725: case 744:
				SpriteManager_barkOnItem(&gSpriteManager, npc,
					GetGameText(1, 201), 3, 15, 0);
				break;
			case 250: case 392:
				if (Item_getNpcNumber(&npc) != 208 && Item_getNpcNumber(&npc) != 210 && Item_getNpcNumber(&npc) != 214)
					SpriteManager_barkOnItem(&gSpriteManager, npc,
						GetGameText(1, GenerateRandomIntegerInRange(4) + 236), 3, 15, 0);
				break;
			case 298:
				SpriteManager_barkOnItem(&gSpriteManager, npc,
					GetGameText(1, 229), 3, 15, 0);
				break;
			default:
				if (!(monster->extraFlags & 0x20))
					SpriteManager_barkOnItem(&gSpriteManager, npc,
						GetGameText(1, GenerateRandomIntegerInRange(3) + 60), 5, 15, 0);
			}
			if (NPC_TYPE(npc) != 250 && NPC_TYPE(npc) != 392 && NPC(&npc)->workType != WORK_WAIT) {
				Npc_setSchedule(&npc, WORK_COMBAT);
				CUR_SCHED(&npc).state = 1;
				Npc_setTarget(&npc, Item_getNpcNumber(&AvatarRef), 1);
			}
			alerted = 1;
		} else {
			if (woken) {
				switch (NPC_TYPE(npc)) {
				case 478: case 354: case 691: case 725: case 744:
					SpriteManager_barkOnItem(&gSpriteManager, npc,
						GetGameText(1, 202), 3, 15, 0);
					break;
				case 250: case 392:
					if (Item_getNpcNumber(&npc) != 208 && Item_getNpcNumber(&npc) != 210 &&
						Item_getNpcNumber(&npc) != 214)
						SpriteManager_barkOnItem(&gSpriteManager, npc,
							GetGameText(1, GenerateRandomIntegerInRange(4) + 236), 3, 15, 0);
					break;
				default:
					if (!(monster->extraFlags & 0x20))
						SpriteManager_barkOnItem(&gSpriteManager, npc,
							GetGameText(1, GenerateRandomIntegerInRange(6) + 149), 3, 15, 0);
				}
			} else {
				switch (NPC_TYPE(npc)) {
				case 478: case 354: case 691: case 725: case 744:
					SpriteManager_barkOnItem(&gSpriteManager, npc,
						GetGameText(1, 203), 3, 15, 0);
					break;
				case 298:
					SpriteManager_barkOnItem(&gSpriteManager, npc,
						GetGameText(1, 228), 3, 15, 0);
					break;
				case 250: case 392:
					if (Item_getNpcNumber(&npc) != 208 && Item_getNpcNumber(&npc) != 210 &&
						Item_getNpcNumber(&npc) != 214)
						SpriteManager_barkOnItem(&gSpriteManager, npc,
							GetGameText(1, GenerateRandomIntegerInRange(4) + 236), 3, 15, 0);
					break;
				default:
					if (!(monster->extraFlags & 0x20))
						SpriteManager_barkOnItem(&gSpriteManager, npc,
							GetGameText(1, GenerateRandomIntegerInRange(3) + 110), 3, 15, 0);
				}
			}
			Npc_setSchedule(&npc, NPC(&npc)->workType);
			if (i < 256)
				CUR_SCHED(&npc).state = 0xff;
			if (NPC(&npc)->workType != WORK_WAIT) {
				Npc_pushSchedule(&npc, WORK_HOUND, -1, -1, 0);
				CUR_SCHED(&npc).state = 1;
			}
		}
	}
	if (noticed) {
		if (hostile && alerted)
			CombatGroups.callGuards(0);
	} else if (runUsable) {
		if (hostile) {
			unsigned char script[128];
			script[0] = 1;
			AppendScriptByte(script, SCRIPT_NO_HALT);
			AppendScriptByte(script, SCRIPT_USECODE);
			AppendScriptWord(script, 0x633);
			ActionQueue.add(thing, (char *)script);
		} else {
			unsigned char script[128];
			script[0] = 1;
			AppendScriptByte(script, SCRIPT_NO_HALT);
			AppendScriptByte(script, SCRIPT_USECODE);
			AppendScriptWord(script, 0x63a);
			ActionQueue.add(thing, (char *)script);
		}
	}
}

void Combat::callGuards(unsigned char quiet)
{
	char n, count, found = 0;
	NPCRef npc;
	int x, y;
	count = CombatGroups.countHostileGuards();
	n = GenerateRandomIntegerInRange(3) + 1;
	GetNpcIbo(&npc, 44);
	if (IsDead(&npc))
		return;
	if (n + count > PartySize + 2)
		n = (int)PartySize - count + 2;
	x = Item_getX(AvatarRef);
	y = Item_getY(AvatarRef);
	if (x > 576 && x < 832 && y > 1200 && y < 1536)
		n = 10;
	if (x > 672 && x < 1136 && y > 2432 && y < 2800)
		n = 4;
	if (n > 0 && ArmageddonDone == 0) {
		GatherRegionalGuards(&found, n, &quiet);
		if (quiet)
			SetGuardsOnAvatar();
		else if (!SpecialMusicPlaying)
			PlayMusic(1);
	}
}

void far GatherRegionalGuards(char *count, char required, unsigned char *quiet)
{
	int i;
	NPCRef npc;
	int x = Item_getX(AvatarRef);
	int y = Item_getY(AvatarRef);
	for (i = 0; i < NPC_COUNT; i++) {
		GetNpcIbo(&npc, i);
		if (!npc.valid() || Item_greatestDeltaToItem(npc, AvatarRef) > 30 || IsInParty(&npc) || IsDead(&npc))
			continue;
		if (IsAsleep(&npc) && NPC(&npc)->workType == WORK_SLEEP)
			NPC(&npc)->changeFlags(NPC_ASLEEP, 0);
		if (IsRegionalGuard(i, quiet) && CUR_SCHED(&npc).kind != WORK_ARREST) {
			if (*count < required && NPC(&npc)->workType != WORK_WAIT) {
				(*count)++;
				NPC(&npc)->setAlignment(3);
				Npc_setTarget(&npc, Item_getNpcNumber(&AvatarRef), 1);
				Npc_pushSchedule(&npc, WORK_ARREST, -1, -1, 0);
			}
			continue;
		}
		char fight = 0;
		char hostile = 0;
		if (x > 1232 && x < 1392 && y > 2160 && y < 2272 && NPC_TYPE(npc) != 228) {
			fight = 1;
			hostile = 1;
		}
		if (x > 848 && x < 1152 && y > 1632 && y < 1968 && NPC_TYPE(npc) != 381) {
			fight = 1;
			if (Item_getNpcNumber(&npc) != 53)
				hostile = 1;
		}
		if (x > 2101 && x < 2496 && y > 1717 && y < 2128) {
			fight = 1;
			if (NPC_TYPE(npc) != 744 && NPC_TYPE(npc) != 725 && NPC_TYPE(npc) != 747)
				hostile = 1;
		}
		if (x > 1941 && x < 2112 && y > 1288 && y < 1392 && i == 35) {
			Npc_setSchedule(&npc, 21);
			CUR_SCHED(&npc).state = 1;
			CombatGroups.spawnGroup(528, 3, required, 0, 10, 1, 0, 0, objref(-1), 0);
			PlaySfx(39, 255, 64);
		}
		if (fight) {
			Npc_setSchedule(&npc, WORK_COMBAT);
			CUR_SCHED(&npc).state = 1;
			Npc_setTarget(&npc, Item_getNpcNumber(&AvatarRef), 1);
		}
		if (hostile) {
			SetAttackMode(npc, ATTACK_FLEE);
			NPC(&npc)->oppressor = Item_getNpcNumber(&AvatarRef);
		}
	}
	if (*count < required &&
		((x > 848 && x < 1152 && y > 1632 && y < 1968) ||
		 (x > 2101 && x < 2496 && y > 1717 && y < 2128) ||
		 (x > 672 && x < 1136 && y > 2432 && y < 2800))) {
		unsigned char script[128];
		script[0] = 1;
		AppendScriptByte(script, SCRIPT_NO_HALT);
		AppendScriptByte(script, SCRIPT_USECODE);
		int action = required - *count + 30000;
		if (!*quiet)
			action += 100;
		AppendScriptWord(script, action);
		ActionQueue.add(80, AvatarRef, (char *)script);
	}
}

unsigned char far IsRegionalGuard(int number, unsigned char *quiet)
{
	NPCRef npc;
	GetNpcIbo(&npc, number);
	int x = Item_getX(AvatarRef);
	int y = Item_getY(AvatarRef);
	if (x > 848 && x < 1152 && y > 1632 && y < 1968 &&
		(NPC_TYPE(npc) == 381 || Item_getNpcNumber(&npc) == 53))
		return 1;
	if (x > 973 && x < 1184 && y > 821 && y < 896) {
		*quiet = 1;
		if (NPC_TYPE(npc) == 862)
			return 1;
	}
	if (x > 1232 && x < 1392 && y > 2160 && y < 2272 && NPC_TYPE(npc) == 228)
		return 1;
	if (x > 672 && x < 1136 && y > 2432 && y < 2800) {
		NPCRef namedGuard;
		GetNpcIbo(&namedGuard, 263);
		if (NPC_TYPE(npc) != 299)
			return 1;
	}
	if (x > 2101 && x < 2496 && y > 1717 && y < 2128 &&
		(NPC_TYPE(npc) == 259 || NPC_TYPE(npc) == 461))
		return 1;
	if (x > 517 && x < 704 && y > 512 && y < 672) {
		*quiet = 1;
		if (NPC_TYPE(npc) == 298)
			return 1;
	}
	if (x > 1941 && x < 2112 && y > 1288 && y < 1392) {
		if (number == 36)
			return 1;
		if (NPC_TYPE(npc) == 528)
			return 1;
	}
	return 0;
}

void far SpawnRegionalGuards(objref *origin, unsigned action)
{
	int x = Item_getX(*origin);
	int y = Item_getY(*origin);
	char hostile, count;
	hostile = 0;
	count = action - 30000;
	if (count > 100) {
		count -= 100;
		hostile = 1;
	}
	if (x > 2101 && x < 2496 && y > 1717 && y < 2128)
		CombatGroups.spawnGroup(259, 3, count, 0, 10, hostile, 0, 0, objref(-1), 0);
	else if (x > 848 && x < 1152 && y > 1632 && y < 1968)
		CombatGroups.spawnGroup(381, 3, count, 0, 10, hostile, 0, 0, objref(-1), 0);
	else if (x > 672 && x < 1136 && y > 2432 && y < 2800)
		CombatGroups.spawnGroup(228, 3, count, 0, 10, hostile, 0, 0, objref(-1), 0);
}

void far SetGuardsOnAvatar()
{
	NPCRef npc;
	int i;
	unsigned char quiet;
	if (!SpecialMusicPlaying)
		PlayMusic(2);
	for (i = 0; i < NPC_COUNT; i++) {
		GetNpcIbo(&npc, i);
		quiet = 1;
		if (npc.valid() && Item_greatestDeltaToItem(npc, AvatarRef) < 31 && !IsInParty(&npc) &&
			IsRegionalGuard(i, &quiet) &&
			i != 36 && !IsInMode(&npc, ATTACK_FLEE) && !IsNpcUnconscious(&npc) && NPC(&npc)->workType != WORK_WAIT) {
			Npc_setSchedule(&npc, WORK_COMBAT);
			CUR_SCHED(&npc).state = 1;
			Npc_setTarget(&npc, Item_getNpcNumber(&AvatarRef), 1);
		}
	}
}

void far DismissRegionalGuards()
{
	NPCRef npc;
	int i;
	for (i = 0; i < NPC_COUNT; i++) {
		GetNpcIbo(&npc, i);
		if (npc.valid() && Item_greatestDeltaToItem(npc, AvatarRef) < 31 && !IsInParty(&npc) &&
			CUR_SCHED(&npc).kind == WORK_ARREST) {
			Npc_popSchedule(&npc, -1);
			Npc_setSchedule(&npc, WORK_WANDER);
		}
	}
}
