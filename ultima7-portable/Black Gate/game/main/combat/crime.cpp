/* Black Gate U7.EXE, overlay segment 223 (file offsets 0x063af0 to 0x0659a7, 7863 bytes).
 * Borland C++ 2.0 -mm -O -G -P -Y rebuilds it byte for byte as C++.
 */

#include "u7port.h"
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
#include "crime.h"

/* the file object that loads and saves the combat state with a game */
struct CombatSaver : DataNode {
	char *name();
	void load(char *);
	void save(char *);
};

#define NPC(p) GetNpcBufferForIbo(p)
inline uint8_t IsParalyzed(objref *p) { return (NPC(p)->status & NPC_PARALYZED) != 0; }
inline uint8_t IsAsleep(objref *p) { return (NPC(p)->status & NPC_ASLEEP) != 0; }
inline uint8_t HasNoHealth(objref *p) { return (int8_t)Item_getHitPoints(p) <= 0; }
#define CUR_SCHED(p) NPC(p)->schedules[NPC(p)->currentSchedule]
inline uint8_t IsKindAtLeast(ItemInfo &info, uint8_t value) { return info.kind() >= value; }
inline uint8_t IsKindAtLeast(ItemInfo &&info, uint8_t value) { return IsKindAtLeast(info, value); }
inline uint8_t IsCharmed(objref *p) { return (NPC(p)->status & NPC_CHARMED) != 0; }
inline uint8_t GetInitialAlignment(objref *p) { return (NPC(p)->status & 0x60) >> 5; }
inline void StartSchedule(NPCRef &npc, int8_t type)
{
	Npc_setSchedule(&npc, type);
	CUR_SCHED(&npc).state = 1;
}
inline void StartSchedule(NPCRef &&npc, int8_t type) { StartSchedule(npc, type); }

uint8_t CrimeUsecodeOff = 0;
Combat CombatGroups;
char *AvFlagsFileName = "av_flags.dat";
CombatSaver AvFlagsFile;
int8_t AvatarInCombat;
uint8_t KillNpcMode;

char *CombatSaver::name()
{
	return AvFlagsFileName;
}

void CombatSaver::load(char *directory)
{
	int8_t partyAttacked, inBattle;
	int16_t enemyHealth, partyHealth;
	int16_t i;
	uint8_t *movePoints;
	DataFile f(BuildPath(directory, AvFlagsFileName, 0), 1);

	KillNpcMode = f.readByte();
	partyAttacked = f.readByte();
	if (partyAttacked)
		CombatGroups.partyAttacked = 1;
	else
		CombatGroups.partyAttacked = 0;
	inBattle = f.readByte();
	if (inBattle)
		CombatGroups.battleMusic = 1;
	else
		CombatGroups.battleMusic = 0;
	enemyHealth = f.readWord();
	partyHealth = f.readWord();
	CombatGroups.enemyHealth = enemyHealth;
	CombatGroups.partyHealth = partyHealth;
	for (i = 0; i < 4; i++)
		CombatGroups.setLeader(f.readWord());
	movePoints = CombatGroups.movePoints;
	for (i = 0; i < NPC_COUNT; i++)
		movePoints[i] = f.readByte();
	AvatarInCombat = f.readByte();
	SchedulePeriod = f.readByte();
}

void CombatSaver::save(char *directory)
{
	int16_t i;
	uint8_t *movePoints;
	DataFile f(BuildPath(directory, AvFlagsFileName, 0), 0);

	f.writeByte(KillNpcMode);
	f.writeByte(CombatGroups.partyAttacked);
	f.writeByte(CombatGroups.battleMusic);
	f.writeWord(CombatGroups.enemyHealth);
	f.writeWord(CombatGroups.partyHealth);
	for (i = 0; i < 4; i++)
		f.writeWord(CombatGroups.leader(i));
	movePoints = CombatGroups.movePoints;
	for (i = 0; i < NPC_COUNT; i++)
		f.writeByte(movePoints[i]);
	f.writeByte(AvatarInCombat);
	f.writeByte(SchedulePeriod);
}

Combat::Combat()
{
	for (int16_t i = 0; i < 4; i++)
		clear(i);
	CombatGroups.clearMovePoints();
	CombatGroups.battleMusic = 0;
}

void Combat::setLeader(int16_t number)
{
	NPCRef npc;

	GetNpcIbo(&npc, number);
	leaders[GetAlignment(&npc)] = number;
}

int16_t Combat::sumGroupHealth(uint8_t group)
{
	NPCRef npc;
	int16_t i, total;

	total = 0;
	for (i = 0; i < NPC_COUNT; i++) {
		GetNpcIbo(&npc, i);
		if (npc.valid() && CanVisit(&npc) && GetAlignment(&npc) == group &&
			(int8_t)Item_getHitPoints(&npc) >= 0)
			total += (int8_t)Item_getHitPoints(&npc);
	}
	return total;
}

int16_t Combat::countGroupWork(uint8_t group, uint8_t work)
{
	NPCRef npc;
	int16_t i, count;

	count = 0;
	for (i = 0; i < NPC_COUNT; i++) {
		GetNpcIbo(&npc, i);
		if (npc.valid() && CanVisit(&npc) && GetAlignment(&npc) == group &&
			NPC(&npc)->workType == work)
			count++;
	}
	return count;
}

void Combat::playEndMusic()
{
	battleMusic = 0;
	if (IsDead(&AvatarRef))
		PlayMusic(17);
	else if (CurrentMusic != 15 && CurrentMusic != 9) {
		if (CombatGroups.partyAttacked) {
			if (CombatGroups.enemyHealth >= CombatGroups.partyHealth)
				PlayMusic(9);
			else
				PlayMusic(15);
		} else
			PlayMusic(18);
	}
	if (CombatGroups.partyAttacked)
		CombatGroups.partyAttacked = 0;
}

void Combat::loseSightOf(int16_t target)
{
	int16_t number;
	NPCRef npc;
	int16_t type;
	objref who(target);
	int16_t i;

	number = Item_getNpcNumber(&who);
	for (i = 0; i < NPC_COUNT; i++) {
		GetNpcIbo(&npc, i);
		if (npc.valid() && CanVisit(&npc) && NPC(&npc)->workType == WORK_COMBAT) {
			type = GetMonsterNumber(npc);
			if (!(uint8_t)MonsterRecords.get(type)->seeInvisible) {
				if (NPC(&npc)->primaryTarget == number)
					Npc_setTarget(&npc, -1, 1);
				if (NPC(&npc)->secondaryTarget == number)
					Npc_setTarget(&npc, -1, 0);
				NPC(&npc)->typeFlagsHigh = NPC(&npc)->typeFlagsHigh | 1;
			}
		}
	}
}

void Combat::releaseProtectors(uint8_t group, int16_t target)
{
	NPCRef npc;
	int16_t primary, secondary, oppressor;
	uint8_t wantedPrimary;
	int16_t i;

	for (i = 0; i < NPC_COUNT; i++) {
		GetNpcIbo(&npc, i);
		if (npc.valid() && CanVisit(&npc) && NPC(&npc)->workType == WORK_COMBAT &&
			GetAlignment(&npc) == group && IsInMode(&npc, ATTACK_PROTECT)) {
			primary = NPC(&npc)->primaryTarget;
			secondary = NPC(&npc)->secondaryTarget;
			oppressor = NPC(&npc)->oppressor;
			wantedPrimary = WantsPrimary(&npc);
			SetAttackMode(npc, ATTACK_NEAREST);
			Npc_setTarget(&npc, primary, 1);
			Npc_setTarget(&npc, secondary, 0);
			NPC(&npc)->oppressor = oppressor;
			if (!HasTarget(&npc) && !HasSecondary(&npc) && target != -1) {
				NPC(&npc)->typeFlagsHigh = NPC(&npc)->typeFlagsHigh & ~1;
				Npc_setTarget(&npc, target, 0);
			} else if (wantedPrimary)
				NPC(&npc)->typeFlagsHigh = NPC(&npc)->typeFlagsHigh | 1;
			else
				NPC(&npc)->typeFlagsHigh = NPC(&npc)->typeFlagsHigh & ~1;
		}
	}
}

int16_t Combat::countHostileGuards()
{
	NPCRef npc;
	int16_t i, count;

	count = 0;
	for (i = 256; i < NPC_COUNT; i++) {
		GetNpcIbo(&npc, i);
		if (npc.valid() && CanVisit(&npc) &&
			(ITEM(npc.off)->typeFrame & 0x3ff) == 946 /* guard */ &&
			GetAlignment(&npc) == 3)
			count++;
	}
	return count;
}

void ReportCrime(int16_t thing, int8_t hostile, int8_t runUsable)
{
	NPCRef npc;
	uint8_t noticed, alerted, woken;
	int16_t monster;
	objref owner;
	int16_t i, type;

	noticed = 0;
	alerted = 0;
	if (InDungeon)
		return;
	for (owner = objref(thing); IsKindAtLeast(GetItemZAndStuff(&owner), 6); owner = Item_getContainer(&owner))
		;
	for (i = 0; i < NPC_COUNT; i++) {
		GetNpcIbo(&npc, i);
		if (!(npc.valid() && CanVisit(&npc) &&
			(Item_greatestDeltaToItem(npc, AvatarRef) <= 20 ||
			owner.valid() && Item_greatestDeltaToItem(npc, owner) <= 20) &&
			!IsInParty(&npc) && !IsInMode(&npc, ATTACK_FLEE) && IsSentient(npc) &&
			GetAlignment(&npc) == 0 && !(uint8_t)(NPC(&npc)->typeFlagsHigh & 0x40) &&
			NPC(&npc)->workType != WORK_WAIT &&
			(type = ITEM(npc.off)->typeFrame & 0x3ff) != 519 /* liche */ &&
			type != 317 /* ghost */ && type != 299 /* ghost */))
			continue;
		if (IsParalyzed(&npc) || HasNoHealth(&npc) || IsDead(&npc))
			continue;
		if (IsAsleep(&npc)) {
			if (NPC(&npc)->workType == WORK_SLEEP && i != 150 && RollChance(3))
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
		if (CUR_SCHED(&npc).kind == WORK_HOUND && RollChance(3) || NPC(&npc)->workType == WORK_COMBAT) {
			alerted = 1;
			hostile = 1;
		}
		monster = GetMonsterNumber(npc);
		if ((uint8_t)(Item_getQualityFlags(&AvatarRef) & QUALITY_INVISIBLE) &&
			!(uint8_t)MonsterRecords.get(monster)->seeInvisible) {
			SpriteManager_barkOnItem(&gSpriteManager, npc,
				GetGameText(1, GenerateRandomIntegerInRange(3) + 133), 3, 15, 0);
			continue;
		}
		if (hostile && !woken) {
			SpriteManager_barkOnItem(&gSpriteManager, npc,
				GetGameText(1, GenerateRandomIntegerInRange(3) + 60), 3, 15, 0);
			Npc_setSchedule(&npc, WORK_COMBAT);
			CUR_SCHED(&npc).state = 1;
			Npc_setTarget(&npc, Item_getNpcNumber(&AvatarRef), 1);
			alerted = 1;
			continue;
		}
		if (woken)
			SpriteManager_barkOnItem(&gSpriteManager, npc,
				GetGameText(1, GenerateRandomIntegerInRange(6) + 149), 3, 15, 0);
		else
			SpriteManager_barkOnItem(&gSpriteManager, npc,
				GetGameText(1, GenerateRandomIntegerInRange(3) + 110), 3, 15, 0);
		Npc_setSchedule(&npc, NPC(&npc)->workType);
		if (i < 256)
			CUR_SCHED(&npc).state = 0xff;
		Npc_pushSchedule(&npc, WORK_HOUND, -1, -1);
		CUR_SCHED(&npc).state = 1;
	}
	if (noticed) {
		if (hostile && alerted)
			CombatGroups.callGuards(0);
	} else if (runUsable && !CrimeUsecodeOff)
		RunUsable(1, thing, hostile ? 0x633 : 0x63a);
}

void Combat::callGuards(uint8_t quiet)
{
	int16_t count, n;

	if (ArmageddonDone)
		return;
	count = CombatGroups.countHostileGuards();
	n = GenerateRandomIntegerInRange(3) + 1;
	if (n + count > PartySize + 2)
		n = PartySize - count + 2;
	if (n > 0) {
		spawnGroup(946 /* guard */, 3, n, 0, 10, !quiet, 0, 0, objref(-1), 0);
		if (quiet)
			SetGuardsOnAvatar();
		else if (!SpecialMusicPlaying)
			PlayMusic(10);
	}
}

void SetGuardsOnAvatar()
{
	NPCRef npc;
	int16_t i;

	if (!SpecialMusicPlaying)
		PlayMusic(11);
	for (i = 256; i < NPC_COUNT; i++) {
		GetNpcIbo(&npc, i);
		if (npc.valid() && CanVisit(&npc) &&
			(ITEM(npc.off)->typeFrame & 0x3ff) == 946 /* guard */ &&
			NPC(&npc)->workType == WORK_COMBAT && !IsNpcUnconscious(&npc) && !IsInMode(&npc, ATTACK_FLEE)) {
			Npc_setSchedule(&npc, WORK_COMBAT);
			CUR_SCHED(&npc).state = 1;
			Npc_setTarget(&npc, Item_getNpcNumber(&AvatarRef), 1);
		}
	}
}

int16_t GetDominantHostileSide()
{
	int16_t result;
	int16_t evil, chaotic;

	result = 0;
	evil = CombatGroups.countGroupWork(2, WORK_COMBAT);
	chaotic = CombatGroups.countGroupWork(3, WORK_COMBAT);
	if (evil > 0 && evil > chaotic)
		result = 1;
	if (chaotic > 0 && chaotic >= evil)
		result = 2;
	return result;
}

int8_t Combat::spawnGroup(int16_t type, int16_t alignment, int16_t count, int16_t work, int16_t mode,
	int8_t hostile, int8_t targeted, int8_t nearby, objref place, int8_t summoned)
{
	int16_t i, spotZ, j, z;
	NPCRef npc;
	Coord x, y, tryX, tryY, avatarX, avatarY;
	int16_t dx, dy;

	for (i = 0; i < count; i++) {
		for (dx = -10; dx < 10; dx++) {
			for (dy = -8; dy < 8; dy++) {
				if (CanTypeMoveTo(Item_getX(AvatarRef) + dx, Item_getY(AvatarRef) + dy,
					0, type)) {
					if (!CreateNpc(&npc, type, Item_getX(AvatarRef) + dx,
						Item_getY(AvatarRef) + dy, 0))
						return 0;
					goto placed;
				}
			}
		}
		return 0;
placed:
		if (nearby) {
			avatarX = Item_getX(AvatarRef);
			avatarY = Item_getY(AvatarRef);
			z = Item_getZ(&AvatarRef);
			for (j = 0; j < 8; j++) {
				tryX = Coord(GenerateRandomIntegerInRange(35) + avatarX - 17);
				tryY = Coord(GenerateRandomIntegerInRange(17) + avatarY - 8);
				tryX = tryX & ~1;
				tryY = tryY & ~1;
				if (CanTypeMoveTo(tryX, tryY, z, type) && HasLineOfFireToCoords(place, tryX, tryY, z)) {
					makeArrivalEffect(tryX, tryY, z, 50, 70);
					Item_move(&npc, tryX, tryY, z);
					break;
				}
			}
			if (j == 8) {
				Item_delete(&npc);
				return 0;
			}
		} else if (!FindSpotNextToItem(&npc, AvatarRef, &x, &y, &spotZ) || !PlaceNpcNearAvatar(&npc, x, y, 0)) {
			Item_delete(&npc);
			return 0;
		}
		RandomizeCreature(npc, ITEM(npc.off)->typeFrame);
		Item_setTemporary(&npc);
		NPC(&npc)->setAlignment(alignment);
		Npc_setSchedule(&npc, work);
		CUR_SCHED(&npc).state = 1;
		if (mode != 10)         /* 10 keeps the default mode */
			SetAttackMode(npc, mode);
		if (hostile) {
			Npc_setTarget(&npc, Item_getNpcNumber(&AvatarRef), 1);
			Npc_pushSchedule(&npc, WORK_ARREST, -1, -1);
		} else if (targeted)
			Npc_setTarget(&npc, Item_getNpcNumber(&AvatarRef), 1);
		if (nearby && !(uint8_t)(Item_getQualityFlags(&npc) & QUALITY_BUSY)) {
			ActionQueue.run(ActionQueue.add(npc, (char *)MakeScript(
				type == 528 /* skeleton */ ? SCRIPT_KNEEL_FRAME : SCRIPT_STAND_FRAME, SCRIPT_END)));
			if (type == 528 /* skeleton */)
				ActionQueue.add(npc, (char *)MakeScript(SCRIPT_WAIT, GenerateRandomIntegerInRange(16) + 15,
					SCRIPT_LIE_FRAME, SCRIPT_WAIT, 3, SCRIPT_BEND_FRAME, SCRIPT_WAIT, 3, SCRIPT_STAND_FRAME,
					SCRIPT_WAIT, 3, SCRIPT_END));
		}
		/* conjured and summoned */
		if (summoned) {
			NPC(&npc)->typeFlagsHigh = NPC(&npc)->typeFlagsHigh | 0x40;
			NPC(&npc)->typeFlagsHigh = NPC(&npc)->typeFlagsHigh | 0x80;
		}
	}
	return 1;
}

void Combat::makeArrivalEffect(Coord x, Coord y, int16_t z, int16_t low, int16_t high)
{
	objref effect;
	int16_t j;
	int16_t dx, dy;
	char frames[4] = {2, 4, 1, 3};

	x = x & ~1;
	y = y & ~1;
	if (CreateItem(&effect, 895 /* fire field */, x, y, z)) {
		Item_setFrame(&effect, 0);
		Item_setTemporary(&effect);
		if (low != -1)
			ActionQueue.add(effect, (char *)MakeScript(SCRIPT_WAIT,
				low <= high ? GenerateRandomIntegerInRange(high - low + 1) + low : high, SCRIPT_REMOVE, SCRIPT_END));
	}
	if (z)
		return;
	dx = GetDelta(x, CellWindowX);
	dy = GetDelta(y, CellWindowY);
	if (!(uint8_t)gItemTypeInfo[CellBuffer[dy][dx] & 0x3ff].water) {
		for (j = 0; j < 8; j += 2) {
			if (CreateItem(&effect, 224 /* dust */, x + DirDeltaX[j], y + DirDeltaY[j], z)) {
				Item_setFrame(&effect, frames[j >> 1]);
				Item_setTemporary(&effect);
			}
		}
		if (CreateItem(&effect, 224 /* dust */, x, y, z)) {
			Item_setFrame(&effect, 5);
			Item_setTemporary(&effect);
		}
	}
}

int16_t GetGroupLeader(uint8_t alignment)
{
	return CombatGroups.leader(alignment);
}

void SetGroupLeader(int16_t number)
{
	CombatGroups.setLeader(number);
}

void ClearGroupLeader(uint8_t alignment)
{
	CombatGroups.clear(alignment);
}

void ReleaseProtectors(uint8_t alignment, int16_t target)
{
	CombatGroups.releaseProtectors(alignment, target);
}

void CharmNpc(objref *npc, uint8_t side)
{
	int16_t type;

	type = MonsterLookup.get(ITEM(npc->off)->typeFrame & 0x3ff);
	if ((uint8_t)MonsterRecords.get(type)->powerSafe || (uint8_t)MonsterRecords.get(type)->charmSafe ||
		IsDead(npc))
		return;
	if (!IsCharmed(npc))
		NPC(npc)->setInitialAlignment(GetAlignment(npc));
	ChangeStatus(npc, 0, 0x100);
	/* dead for a moment, so every attacker lets go of it */
	ChangeStatus(npc, 0, 0x8000);
	RemoveFromCombat(*npc);
	ChangeStatus(npc, 0x8000, 0);
	NPC(npc)->setAlignment(side);
	if (NPC(npc)->workType == WORK_COMBAT)
		StartSchedule(*npc, WORK_COMBAT);
}

void UncharmNpc(objref *npc)
{
	if (!IsCharmed(npc) || IsDead(npc))
		return;
	ChangeStatus(npc, 0x100, 0);
	/* dead for a moment, so every attacker lets go of it */
	ChangeStatus(npc, 0, 0x8000);
	RemoveFromCombat(*npc);
	ChangeStatus(npc, 0x8000, 0);
	NPC(npc)->setAlignment(GetInitialAlignment(npc));
	if (NPC(npc)->workType == WORK_COMBAT)
		StartSchedule(*npc, WORK_COMBAT);
}
