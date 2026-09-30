/* Black Gate U7.EXE, overlay segment 331 (file offsets 0x09ad80 to 0x09b72e, 2478 bytes).
 * Borland C++ 2.0 -mm -O -P rebuilds it byte for byte as C++.
 * Folder inferred from Serpent Isle's link groups.
 */

#include "activity.h"
#include "lowlevel.h"
#include "ucvalue.h"
#include "uclist.h"
#include "u7npc.h"
#include "item.h"
#include "voolook.h"
#include "collide.h"
#include "daze.h"
#include "actutil.h"
#include "barge.h"
#include "crime.h"
#include "makemojo.h"
#include "monsters.h"
#include "type.h"
#include "npcref.h"
#include "midiplay.h"

/* the flag numbers usecode tests, sets and clears */
#define ITEM_FLAG_INVISIBLE         0
#define ITEM_FLAG_ASLEEP            1
#define ITEM_FLAG_CHARMED           2
#define ITEM_FLAG_CURSED            3
#define ITEM_FLAG_DEAD              4
#define ITEM_FLAG_IN_PARTY          6
#define ITEM_FLAG_PARALYZED         7
#define ITEM_FLAG_POISONED          8
#define ITEM_FLAG_PROTECTED         9
#define ITEM_FLAG_ON_MOVING_BARGE   10
#define ITEM_FLAG_OKAY_TO_TAKE      11
#define ITEM_FLAG_MIGHT             12
#define ITEM_FLAG_POWER_SAFE        13
#define ITEM_FLAG_CANT_DIE          14
#define ITEM_FLAG_IN_ACTION         15
#define ITEM_FLAG_DONT_MOVE         16
#define ITEM_FLAG_TEMPORARY         18
#define ITEM_FLAG_ACTIVE_SAILOR     20
#define ITEM_FLAG_OKAY_TO_LAND      21
#define ITEM_FLAG_MUSIC_DEVICE      22
#define ITEM_FLAG_IN_DUNGEON        23
#define ITEM_FLAG_SOLID             24
#define ITEM_FLAG_OINK_MODE         25
#define ITEM_FLAG_ACTIVE_BARGE      26
#define ITEM_FLAG_FREE_NPC_NUMBERS  27

inline void SetExtended(NPCRef &npc, unsigned char bits)
{
	npc.buffer()->typeFlagsHigh = npc.buffer()->typeFlagsHigh | bits;
}
inline void ClearExtended(NPCRef &npc, unsigned char bits)
{
	npc.buffer()->typeFlagsHigh = npc.buffer()->typeFlagsHigh & ~bits;
}
inline void ChangeActivity(NPCRef &npc, char activity)
{
	Npc_setSchedule(&npc, activity);
	npc.buffer()->schedules[npc.buffer()->currentSchedule].state = 1;
}

struct MonsterRef {
	int index;
	MonsterRecord *operator->() { return MonsterRecords.get(index); }
};

extern unsigned char AvatarDontMove;
extern unsigned char OinkMode;
extern int ActiveSailor;
extern int CurrentVehicle;
extern int ActiveBarge;

#define NPC_FLAG(r, bits) ((unsigned char)((NPCRef(r).buffer()->status & (bits)) != 0))

/* the int in element i of the usecode value at v */
#define ARG(v, i)   GetListNode(v, i)->toInt()

/* Usecode engine calls: args - 1 is the first argument, ret takes the result. */

/* 0x88: one of an item's flags */
void far UC_GetItemFlag(Value *args, Value *ret)
{
	int result = 0;
	int object = GetItemRef(GetListNode(args - 1, 1));
	objref ref = object;
	MonsterRef monster;
	unsigned char flag = ARG(args - 2, 1);

	switch (flag) {
	case ITEM_FLAG_INVISIBLE:
		result = (unsigned char)(Item_getQualityFlags(&ref) & QUALITY_INVISIBLE);
		break;
	case ITEM_FLAG_ASLEEP:
		result = NPC_FLAG(ref, NPC_ASLEEP);
		break;
	case ITEM_FLAG_CHARMED:
		result = NPC_FLAG(ref, NPC_CHARMED);
		break;
	case ITEM_FLAG_CURSED:
		result = NPC_FLAG(ref, NPC_CURSED);
		break;
	case ITEM_FLAG_DEAD:
		result = NPC_FLAG(ref, NPC_DEAD);
		break;
	case 5:
		result = NPC_FLAG(ref, 0x400);
		break;
	case ITEM_FLAG_IN_PARTY:
		result = NPC_FLAG(ref, NPC_IN_PARTY);
		break;
	case ITEM_FLAG_PARALYZED:
		result = NPC_FLAG(ref, NPC_PARALYZED);
		break;
	case ITEM_FLAG_POISONED:
		result = NPC_FLAG(ref, NPC_POISONED);
		break;
	case ITEM_FLAG_PROTECTED:
		result = NPC_FLAG(ref, NPC_PROTECTED);
		break;
	case ITEM_FLAG_ON_MOVING_BARGE:
		result = CurrentVehicle;
		break;
	case ITEM_FLAG_OKAY_TO_TAKE:
		result = Item_isOkayToTake(&ref);
		break;
	case ITEM_FLAG_POWER_SAFE:
		monster.index = GetMonsterNumber(ref);
		result = (unsigned char)monster->powerSafe;
		break;
	case ITEM_FLAG_CANT_DIE:
		monster.index = GetMonsterNumber(ref);
		result = (unsigned char)monster->deathSafe;
		break;
	case ITEM_FLAG_IN_ACTION:
		result = (unsigned char)(NPCRef(ref).buffer()->typeFlagsHigh & 0x20);
		break;
	case ITEM_FLAG_DONT_MOVE:
		result = AvatarDontMove;
		break;
	case ITEM_FLAG_TEMPORARY:
		result = IsTemporary(&ref);
		break;
	case 19:
		result = (unsigned char)(Item_getQualityFlags(&ref) & 2);
		break;
	case ITEM_FLAG_ACTIVE_SAILOR:
		result = ActiveSailor;
		break;
	case ITEM_FLAG_OKAY_TO_LAND:
		result = (unsigned char)Barge_isOkayToLand(NPCRef(ref));
		break;
	case ITEM_FLAG_MUSIC_DEVICE:
		result = MusicDevice;
		break;
	case ITEM_FLAG_IN_DUNGEON:
		result = InDungeon;
		break;
	case ITEM_FLAG_SOLID:
		result = (unsigned char)gItemTypeInfo[ITEM(ref.off)->typeFrame & 0x3ff].solid;
		break;
	case ITEM_FLAG_OINK_MODE:
		result = OinkMode;
		break;
	case ITEM_FLAG_ACTIVE_BARGE:
		result = ActiveBarge;
		break;
	case ITEM_FLAG_FREE_NPC_NUMBERS:
		result = FreeNpcNumbers;
		break;
	}
	ret->appendInt(result);
}

/* 0x89 */
void far UC_SetItemFlag(Value *args, Value *)
{
	int object = GetItemRef(GetListNode(args - 1, 1));
	objref ref = object;
	unsigned char flag = ARG(args - 2, 1);

	switch (flag) {
	case ITEM_FLAG_INVISIBLE:
		Item_setInvisible(&ref);
		break;
	case ITEM_FLAG_ASLEEP:
		PutToSleep(ref);
		break;
	case ITEM_FLAG_CHARMED:
		CharmNpc(&NPCRef(ref), 1);
		break;
	case ITEM_FLAG_CURSED:
		ApplyCurse(ref);
		break;
	case ITEM_FLAG_DEAD:
		NPCRef(ref).changeFlags(0, NPC_DEAD);
		break;
	case 5:
		NPCRef(ref).changeFlags(0, 0x400);
		break;
	case ITEM_FLAG_PARALYZED:
		ApplyParalysis(ref);
		break;
	case ITEM_FLAG_POISONED:
		ApplyPoison(ref);
		break;
	case ITEM_FLAG_PROTECTED:
		NPCRef(ref).changeFlags(0, NPC_PROTECTED);
		break;
	case ITEM_FLAG_ON_MOVING_BARGE:
		CurrentVehicle = object;
		break;
	case ITEM_FLAG_OKAY_TO_TAKE:
		Item_setOkayToTake(&ref);
		break;
	case ITEM_FLAG_MIGHT:
		SetExtended(NPCRef(ref), 0x10);
		break;
	case ITEM_FLAG_IN_ACTION:
		SetExtended(NPCRef(ref), 0x20);
		break;
	case ITEM_FLAG_DONT_MOVE:
		AvatarDontMove = 1;
		break;
	case ITEM_FLAG_TEMPORARY:
		Item_setTemporary(&ref);
		break;
	case 19:
		Item_setQualityFlags(&ref, Item_getQualityFlags(&ref) | 2);
		break;
	case ITEM_FLAG_ACTIVE_SAILOR:
		ActiveSailor = object;
		break;
	case ITEM_FLAG_OINK_MODE:
		OinkMode = 1;
		break;
	case ITEM_FLAG_ACTIVE_BARGE:
		ActiveBarge = object;
		break;
	}
}

/* 0x8a */
void far UC_ClearItemFlag(Value *args, Value *)
{
	int object = GetItemRef(GetListNode(args - 1, 1));
	objref ref = object;
	unsigned char flag = ARG(args - 2, 1);

	switch (flag) {
	case ITEM_FLAG_INVISIBLE:
		Item_setQualityFlags(&ref, Item_getQualityFlags(&ref) & ~QUALITY_INVISIBLE);
		break;
	case ITEM_FLAG_ASLEEP:
		if (NPCRef(ref).buffer()->workType == WORK_SLEEP)
			ChangeActivity(NPCRef(ref), WORK_LOITER);
		WakeUpNpc(&NPCRef(ref));
		break;
	case ITEM_FLAG_CHARMED:
		UncharmNpc(&NPCRef(ref));
		break;
	case ITEM_FLAG_CURSED:
		EndCurse(ref);
		break;
	case ITEM_FLAG_DEAD:
		NPCRef(ref).changeFlags(NPC_DEAD, 0);
		break;
	case 5:
		NPCRef(ref).changeFlags(0x400, 0);
		break;
	case ITEM_FLAG_PARALYZED:
		EndParalysis(ref);
		break;
	case ITEM_FLAG_POISONED:
		EndPoison(ref);
		break;
	case ITEM_FLAG_PROTECTED:
		NPCRef(ref).changeFlags(NPC_PROTECTED, 0);
		break;
	case ITEM_FLAG_ON_MOVING_BARGE:
		CurrentVehicle = 0;
		break;
	case ITEM_FLAG_OKAY_TO_TAKE:
		Item_clearOkayToTake(&ref);
		break;
	case ITEM_FLAG_MIGHT:
		ClearExtended(NPCRef(ref), 0x10);
		break;
	case ITEM_FLAG_IN_ACTION:
		ClearExtended(NPCRef(ref), 0x20);
		break;
	case ITEM_FLAG_DONT_MOVE:
		AvatarDontMove = 0;
		break;
	case 19:
		Item_setQualityFlags(&ref, Item_getQualityFlags(&ref) & ~2);
		break;
	case ITEM_FLAG_ACTIVE_SAILOR:
		ActiveSailor = 0;
		break;
	case ITEM_FLAG_OINK_MODE:
		OinkMode = 0;
		break;
	case ITEM_FLAG_ACTIVE_BARGE:
		ActiveBarge = 0;
		break;
	}
}
