/* Serpent Isle SI.EXE, overlay segment 248 (file offsets 0x06b890 to 0x06bd1b, 1163 bytes).
 * Borland C++ 2.0 -mm -O -G -P -d rebuilds it byte for byte as C++.
 */

/* path: u7npc.c */
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
#include "worldpal.h"
#include "init.h"
#include "gtimer.h"

#define NPC(p) GetNpcBufferForIbo(p)
#define CUR_SCHED(p) (NPC(p)->schedules[NPC(p)->currentSchedule])
#define TYPE(rec) ((rec)->typeFrame & 0x3ff)
#define IS_VALID(r) ((int8_t) ((r).off != 0))
#define NPC_COUNT 356

/* The U7NBUF.DAT file in a saved game: every NPC buffer and equip list. */
struct NpcSaver : DataNode {
	int16_t unused;
	char *name();
	void load(char *dir);
	void save(char *dir);
	void refresh(char *dir);
};

extern int16_t NpcItemRefs[];


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
	WriteHandleFromVoodoo(file, -INT32_C(1), (uint16_t) ((NpcRecordCount + ExtraNpcRecordCount) * 36), EquipList);
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
	ReadHandleToVoodoo(file, -INT32_C(1), (uint16_t)((NpcRecordCount + ExtraNpcRecordCount) * 36), &EquipList.address);
	DosClose(file);
}

/* Finds the avatar among the NPCs and gives its shape the skin colour and sex chosen. */
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
				if (Npc_hasPolymorphFlag(&AvatarRef))
					break;
				if (!Npc_isMale(&AvatarRef)) {
					switch (Npc_getSkinColor(&handle)) {
					case 0: RemapShapeRecord(721, 989); break;
					case 1: RemapShapeRecord(721, 1027); break;
					case 2: RemapShapeRecord(721, 1025); break;
					case 3: FatalError("Not supported yet!", __FILE__, 169); break;
					default: FatalError("Unknown skin color!", __FILE__, 172); break;
					}
					CopyWihhEntry(721, 989);
				} else {
					switch (Npc_getSkinColor(&handle)) {
					case 0: RemapShapeRecord(721, 721); break;
					case 1: RemapShapeRecord(721, 1026); break;
					case 2: RemapShapeRecord(721, 1024); break;
					case 3: FatalError("Not supported yet!", __FILE__, 198); break;
					default: FatalError("Unknown skin color!", __FILE__, 201); break;
					}
					CopyWihhEntry(721, 721);
				}
				break;
			}
		}
	}
	return found;
}

void ResetNpcSchedules(void)
{
	int16_t npcNum;
	objref npc;
	uint8_t workType;
	uint8_t previous;

	for (npcNum = 0; npcNum < NPC_COUNT; npcNum++) {
		GetNpcIbo(&npc, npcNum);
		if (IS_VALID(npc)) {
			previous = NPC(&npc)->workType;
			if (npcNum < 256 && NPC(&npc)->workType != WORK_WAIT) {
				workType = Schedule_getWorkType(&ScheduleTable, npcNum, GameTime.getHour() / 3);
				if (workType == 255) {
					Npc_setSchedule(&npc, previous);
					CUR_SCHED(&npc).state = 1;
				} else
					Npc_setSchedule(&npc, workType);
			} else {
				Npc_setSchedule(&npc, previous);
				CUR_SCHED(&npc).state = 1;
			}
			NPC(&npc)->food = 18;
		}
	}
	KillNpcMode = 0;
}
