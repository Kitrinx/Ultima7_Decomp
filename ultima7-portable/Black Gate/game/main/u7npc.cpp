/* Black Gate U7.EXE, resident segment 62 (file offsets 0x024fdf to 0x025c98, 3257 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "u7port.h"
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

#define NPC(p) GetNpcBufferForIbo(p)
#define CURRENT(p) (NPC(p)->schedules[NPC(p)->currentSchedule])
#define NPC_FLAG(r, mask) ((uint8_t)((NPC(r)->status & (mask)) != 0))
#define TYPE(rec) ((rec)->typeFrame & 0x3ff)
#define VISIBLE(r) ((int8_t)(GetItemKind(r) == LOCATION_OFF_MAP) ? 0 : IsItemInCellWindow((r)->off))

/* IsInCombat on a copy of the reference. */
inline uint8_t IsFighting(NPCRef r)
{
	NPCRef *p = &r;
	return NPC(p)->workType == WORK_COMBAT && IsOnOutermostSchedule(p);
}

/* The U7NBUF.DAT file in a saved game: every NPC buffer and equip list. */
struct NpcSaver : DataNode {
	int16_t unused;
	char *name();
	void load(char *dir);
	void save(char *dir);
	void refresh(char *dir);
};

extern int16_t NpcItemRefs[];

char *U7NBufFileName = "U7NBUF.DAT";
NpcSaver NpcBufferFile;

char *NpcSaver::name()
{
	return U7NBufFileName;
}

void NpcSaver::save(char *dir)
{
	int16_t file;
	int16_t i;

	file = CreateFileOrFail(BuildPath(dir, U7NBufFileName, 0));
	for (i = 0; i < NpcRecordCount + ExtraNpcRecordCount; i++) {
		GetCachedNpcBuffer(&NpcPool, i)->ref = NpcItemRefs[i];
		DosWrite(file, -INT32_C(1), (int32_t)sizeof(NpcBuffer), GetCachedNpcBuffer(&NpcPool, i));
	}
	DosWrite(file, -INT32_C(1), INT32_C(2), &FreeNpcNumbers);
	DosWrite(file, -INT32_C(1), INT32_C(2), &FreeMonsterNumbers);
	WriteHandleFromVoodoo(file, -INT32_C(1), (uint16_t) ((NpcRecordCount + ExtraNpcRecordCount) * 24), EquipList);
	DosClose(file);
}

void NpcSaver::refresh(char *directory)
{
	ResetNpcSchedules();
	save(directory);
}

void NpcSaver::load(char *directory)
{
	int16_t file;
	int16_t i;
	file = OpenFileOrFail(BuildPath(directory, U7NBufFileName, 0));
	for (i = 0; i < NpcRecordCount + ExtraNpcRecordCount; i++) {
		DosRead(file, -INT32_C(1), (int32_t)sizeof(NpcBuffer), GetCachedNpcBuffer(&NpcPool, i));
		NpcItemRefs[i] = GetCachedNpcBuffer(&NpcPool, i)->ref;
	}
	DosRead(file, -INT32_C(1), INT32_C(2), &FreeNpcNumbers);
	DosRead(file, -INT32_C(1), INT32_C(2), &FreeMonsterNumbers);
	ReadHandleToVoodoo(file, -INT32_C(1), (uint16_t)((NpcRecordCount + ExtraNpcRecordCount) * 24), &EquipList.address);
	DosClose(file);
}

uint8_t FindAvatarNpc(void)
{
	uint8_t found;
	objref handle;
	int16_t i;

	found = 0;
	for (i = 0; i < NpcRecordCount; i++) {
		GetNpcIbo(&handle, i);
		if (handle.valid()) {
			if (TYPE(ITEM(handle.off)) == 721 || TYPE(ITEM(handle.off)) == 989) { /* Avatar */
				AvatarRef = handle;
				found = 1;
				if (!Npc_isMale(&AvatarRef)) {
					RemapShapeRecord(721, 989); /* Avatar */
					CopyWihhEntry(721, 989);    /* Avatar */
				} else {
					RemapShapeRecord(721, 721); /* Avatar */
					CopyWihhEntry(721, 721);    /* Avatar */
				}
				break;
			}
		}
	}
	return found;
}

void Npc_clearTargets(objref *who)
{
	GetNpcBufferForIbo(who)->primaryTarget = -1;
	GetNpcBufferForIbo(who)->secondaryTarget = -1;
	GetNpcBufferForIbo(who)->oppressor = -1;
}

/* Starts a new work type: resets the schedule stack, picks the attack mode (party members keep
 * theirs), and on entering combat readies a weapon, may bark a battle cry and starts battle music. */
void Npc_setSchedule(objref *who, int8_t activity)
{
	int16_t i, type, grade;
	int16_t unusedWeapon = GetNpcWeapon(&NPCRef(*who));
	objref other;
	uint8_t bark;

	if ((uint8_t)Item_isAvatar(who)) {
		if (activity == WORK_FOLLOW_AVT)
			ResetPartyFormation();
		AvatarInCombat = activity == WORK_COMBAT;
		if (NPC(who)->workType == WORK_COMBAT && activity == WORK_FOLLOW_AVT && CombatGroups.countEnemies())
			PlayMusic(16);
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
	if (NPC_FLAG(who, NPC_IN_PARTY) || (uint8_t)(NPC(who)->typeFlagsHigh & 0x40)) {
		Item_setQuality(who, NPC(who)->defaultAttackMode);
	} else {
		grade = 0;
		type = MonsterLookup.get(TYPE(ITEM(who->off)));
		for (; grade < 3; grade++) {
			if (!GenerateRandomIntegerInRange(
				CategoryAttackOdds[(uint8_t)(MonsterRecords.get(type)->category - 1)][grade]))
				break;
		}
		Item_setQuality(who,
			CategoryAttackModes[(uint8_t)(MonsterRecords.get(type)->category - 1)][grade]);
		NPC(who)->defaultAttackMode =
			CategoryAttackModes[(uint8_t)(MonsterRecords.get(type)->category - 1)][grade];
	}
	Npc_clearTargets(who);
	if (activity == WORK_COMBAT) {
		NPC(who)->typeFlagsHigh = NPC(who)->typeFlagsHigh & ~4;
		NPC(who)->face = 0;
		SelectWeapon(*who, 0, 0);
		if (!(uint8_t)Item_isAvatar(who) && RollChance(3)) {
			if (NPC_FLAG(who, NPC_IN_PARTY) || (uint8_t)(NPC(who)->typeFlagsHigh & 0x40)) {
				bark = 0;
				for (i = 0; i < NPC_COUNT; i++) {
					GetNpcIbo(&other, i);
					if (other.valid() && VISIBLE(&other) &&
						!NPC_FLAG(&other, NPC_IN_PARTY) && !(uint8_t)(NPC(&other)->typeFlagsHigh & 0x40) &&
						NPC(&other)->workType == WORK_COMBAT) {
						bark = 1;
						break;
					}
				}
			} else
				bark = 1;
			if (bark)
				SpriteManager_barkOnItem(&gSpriteManager, who->off,
					GetGameText(1, GenerateRandomIntegerInRange(3) + 57), 3, 15, 0);
		}
		ActionQueue.remove(who->off, 1, 0);
		if (NPC(who)->workType == WORK_THIEF)
			NPC(who)->setAlignment(2);
		if (!NPC_FLAG(who, NPC_DEAD) && VISIBLE(who) &&
			(GetAlignment(who) == 3 || GetAlignment(who) == 2)) {
			if (!(int16_t)SpecialMusicPlaying)
				PlayMusic(10);
			CombatGroups.battleMusic = 1;
		}
	}
	StopPaths(*who);
	NPC(who)->workType = activity;
}

void Npc_runSchedule(objref *who)
{
	objref ref;

	if (CURRENT(who).kind <= 0)
		return;
	if (CURRENT(who).kind >= 53)
		return;
	if (WorkTypeHandlers[CURRENT(who).kind] != 0) {
		ref = objref(who->off);
		(*WorkTypeHandlers[CURRENT(who).kind])(&ref);
	}
}

void Npc_pushSchedule(objref *who, int8_t activity, int16_t x, int16_t y)
{
	if (NPC(who)->currentSchedule < 5) {
		NPC(who)->currentSchedule++;
		CURRENT(who).kind = activity;
		CURRENT(who).state = 0;
		CURRENT(who).x = x;
		CURRENT(who).y = y;
	}
}

void Npc_popSchedule(objref *who, int16_t result)
{
	NPC(who)->result = result;
	if (NPC(who)->currentSchedule > 0) {
		NPC(who)->currentSchedule--;
		if (!IsFighting(*who))
			CURRENT(who).state++;
	}
}
