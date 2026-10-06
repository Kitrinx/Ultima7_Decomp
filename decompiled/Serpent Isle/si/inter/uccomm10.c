/* Serpent Isle SI.EXE, overlay segment 311 (file offsets 0x088310 to 0x089601, 4849 bytes).
 * Borland C++ 2.0 -mm -O -P rebuilds it byte for byte as C++.
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
#include "item.h"
#include "itemrec.h"
#include "u7ibuf.h"
#include "target.h"
#include "sche.h"
#include "movepath.h"
#include "weather.h"
#include "tools.h"

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
#define ITEM_FLAG_IN_DUNGEON        23
#define ITEM_FLAG_SOLID             24
#define ITEM_FLAG_OINK_MODE         25
#define ITEM_FLAG_ACTIVE_BARGE      26
#define ITEM_FLAG_FREE_NPC_NUMBERS  27
#define ITEM_FLAG_MET               28
#define ITEM_FLAG_TOURNAMENT        29
#define ITEM_FLAG_ZOMBIE            30
#define ITEM_FLAG_NO_SPELL_CASTING  31
#define ITEM_FLAG_POLYMORPH         32
#define ITEM_FLAG_TATTOOED          33
#define ITEM_FLAG_READ              34
#define ITEM_FLAG_PETRA             35
#define ITEM_FLAG_FLY               36
#define ITEM_FLAG_FREEZE            37

inline void SetExtended(NPCRef &npc, unsigned char bits)
{
	npc.buffer()->typeFlagsHigh = npc.buffer()->typeFlagsHigh | bits;
}
inline void ClearExtended(NPCRef &npc, unsigned char bits)
{
	npc.buffer()->typeFlagsHigh = npc.buffer()->typeFlagsHigh & ~bits;
}
inline void SetMovement(NPCRef &npc, unsigned char bits)
{
	npc.buffer()->typeFlags = npc.buffer()->typeFlags | bits;
}
inline void ClearMovement(NPCRef &npc, unsigned char bits)
{
	npc.buffer()->typeFlags = npc.buffer()->typeFlags & ~bits;
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

extern int ActiveSailor;
extern int ActiveBarge;
extern unsigned char PendingSteps;

int GetClothingWarmth(objref npc);

#define NPC_FLAG(r, bits) ((unsigned char)((NPCRef(r).buffer()->status & (bits)) != 0))

/* the int in element i of the usecode value at v */
#define ARG(v, i)   GetListNode(v, i)->toInt()

/* Usecode engine calls: args - 1 is the first argument, ret takes the result. */

/* 0xa3: one of an item's flags */
void far UC_GetItemFlag(Value *args, Value *ret)
{
	int result = 0;
	int object = GetItemRef(GetListNode(args - 1, 1));
	objref ref = object;
	MonsterRef monster;
	unsigned char flag = ARG(args - 2, 1);

	if (!ref.valid()) {
		ret->appendInt(0);
		return;
	}
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
	case ITEM_FLAG_MET:
		result = Npc_hasMetFlag(&NPCRef(ref));
		break;
	case ITEM_FLAG_TOURNAMENT:
		result = Npc_hasTournamentFlag(&NPCRef(ref));
		break;
	case ITEM_FLAG_ZOMBIE:
		result = Npc_hasZombieFlag(&NPCRef(ref));
		break;
	case ITEM_FLAG_NO_SPELL_CASTING:
		result = Npc_hasNoCastFlag(&NPCRef(ref));
		break;
	case ITEM_FLAG_POLYMORPH:
		result = Npc_hasPolymorphFlag(&NPCRef(ref));
		break;
	case ITEM_FLAG_TATTOOED:
		result = Npc_hasCombatLowFlag(&NPCRef(ref));
		break;
	case ITEM_FLAG_READ:
		if (NPCRef(ref).isAvatar())
			result = Npc_hasReadFlag(&NPCRef(ref));
		break;
	case ITEM_FLAG_PETRA:
		if (NPCRef(ref).isAvatar())
			result = Npc_hasPetraFlag(&NPCRef(ref));
		break;
	case ITEM_FLAG_FLY:
		result = (unsigned char)(NPCRef(ref).buffer()->typeFlags & NPC_FLY);
		break;
	case ITEM_FLAG_FREEZE:
		if (NPCRef(ref).isAvatar())
			result = Npc_hasFreezeFlag(&NPCRef(ref));
		break;
	}
	ret->appendInt(result);
}

/* 0xa4: set one of an item's flags */
void far UC_SetItemFlag(Value *args, Value *)
{
	int object = GetItemRef(GetListNode(args - 1, 1));
	objref ref = object;
	unsigned char flag = ARG(args - 2, 1);

	if (!ref.valid())
		return;
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
		NPCRef(ref).buffer()->changeFlags(0, NPC_DEAD);
		break;
	case 5:
		NPCRef(ref).buffer()->changeFlags(0, 0x400);
		break;
	case ITEM_FLAG_PARALYZED:
		ApplyParalysis(ref);
		break;
	case ITEM_FLAG_POISONED:
		ApplyPoison(ref);
		break;
	case ITEM_FLAG_PROTECTED:
		NPCRef(ref).buffer()->changeFlags(0, NPC_PROTECTED);
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
		if (!AvatarDontMove) {
			AvatarDontMove = 1;
			PendingSteps = 0;
			FlushPlayerInput();
			StopAutoroute(0);
		}
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
	case ITEM_FLAG_MET:
		if (NPCRef(ref).isAvatar())
			break;
		Npc_setMetFlag(&NPCRef(ref));
		break;
	case ITEM_FLAG_TOURNAMENT:
		Npc_setTournamentFlag(&NPCRef(ref));
		break;
	case ITEM_FLAG_ZOMBIE:
		if (NPCRef(ref).isAvatar())
			break;
		Npc_setZombieFlag(&NPCRef(ref));
		break;
	case ITEM_FLAG_NO_SPELL_CASTING:
		if (NPCRef(ref).isAvatar())
			break;
		Npc_setNoCastFlag(&NPCRef(ref));
		break;
	case ITEM_FLAG_POLYMORPH:
		Npc_setPolymorphFlag(&NPCRef(ref));
		break;
	case ITEM_FLAG_TATTOOED:
		if (!NPCRef(ref).isAvatar())
			break;
		Npc_setCombatLowFlag(&NPCRef(ref));
		break;
	case ITEM_FLAG_READ:
		if (!NPCRef(ref).isAvatar())
			break;
		if (NPCRef(ref).isAvatar())
			Npc_setReadFlag(&NPCRef(ref));
		break;
	case ITEM_FLAG_PETRA:
		if (NPCRef(ref).isAvatar())
			Npc_setPetraFlag(&NPCRef(ref));
		break;
	case ITEM_FLAG_FLY:
		SetMovement(NPCRef(ref), NPC_FLY);
		break;
	case ITEM_FLAG_FREEZE:
		Npc_setFreezeFlag(&NPCRef(ref));
		break;
	}
}

/* 0xa5: clear one of an item's flags */
void far UC_ClearItemFlag(Value *args, Value *)
{
	int object = GetItemRef(GetListNode(args - 1, 1));
	objref ref = object;
	unsigned char flag = ARG(args - 2, 1);

	if (!ref.valid())
		return;
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
		NPCRef(ref).buffer()->changeFlags(NPC_DEAD, 0);
		break;
	case 5:
		NPCRef(ref).buffer()->changeFlags(0x400, 0);
		break;
	case ITEM_FLAG_PARALYZED:
		EndParalysis(ref);
		break;
	case ITEM_FLAG_POISONED:
		EndPoison(ref);
		break;
	case ITEM_FLAG_PROTECTED:
		NPCRef(ref).buffer()->changeFlags(NPC_PROTECTED, 0);
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
		if (AvatarDontMove == 2)
			UpdateNpcSchedules();
		AvatarDontMove = 0;
		FlushPlayerInput();
		break;
	case ITEM_FLAG_TEMPORARY:
		Item_clearTemporary(&ref);
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
	case ITEM_FLAG_MET:
		if (NPCRef(ref).valid())
			Npc_clearMetFlag(&NPCRef(ref));
		break;
	case ITEM_FLAG_TOURNAMENT:
		if (NPCRef(ref).valid())
			Npc_clearTournamentFlag(&NPCRef(ref));
		break;
	case ITEM_FLAG_ZOMBIE:
		if (NPCRef(ref).valid())
			Npc_clearZombieFlag(&NPCRef(ref));
		break;
	case ITEM_FLAG_NO_SPELL_CASTING:
		if (NPCRef(ref).valid())
			Npc_clearNoCastFlag(&NPCRef(ref));
		break;
	case ITEM_FLAG_POLYMORPH:
		if (NPCRef(ref).valid())
			Npc_clearPolymorphFlag(&NPCRef(ref));
		break;
	case ITEM_FLAG_TATTOOED:
		if (NPCRef(ref).valid())
			Npc_clearCombatLowFlag(&NPCRef(ref));
		break;
	case ITEM_FLAG_READ:
		if (NPCRef(ref).isAvatar())
			Npc_clearReadFlag(&NPCRef(ref));
		break;
	case ITEM_FLAG_PETRA:
		if (NPCRef(ref).isAvatar())
			Npc_clearPetraFlag(&NPCRef(ref));
		break;
	case ITEM_FLAG_FLY:
		ClearMovement(NPCRef(ref), NPC_FLY);
		break;
	case ITEM_FLAG_FREEZE:
		Npc_clearFreezeFlag(&NPCRef(ref));
		break;
	}
}

/* 0xb8: an NPC's body temperature */
void far UC_GetTemperature(Value *args, Value *ret)
{
	int object = GetItemRef(GetListNode(args - 1, 1));
	objref ref = object;

	if (ref.valid()) {
		unsigned char temperature = Npc_getTemperature(&ref);
		ret->appendInt(temperature);
	} else
		ret->appendInt(0);
}

/* 0xb9: how cold an NPC is, from 2 to 5; 0 for no item */
void far UC_GetTemperatureZone(Value *args, Value *ret)
{
	int object = GetItemRef(GetListNode(args - 1, 1));
	objref ref = object;

	if (!ref.valid())
		ret->appendInt(0);
	else if (Npc_packedManaLowRange(&ref))
		ret->appendInt(2);
	else if (Npc_packedManaMiddleRange(&ref))
		ret->appendInt(3);
	else if (Npc_packedManaUpperRange(&ref))
		ret->appendInt(4);
	else if (Npc_packedManaHighRange(&ref))
		ret->appendInt(5);
}

/* 0xba: set an NPC's body temperature */
void far UC_SetTemperature(Value *args, Value *)
{
	int object = GetItemRef(GetListNode(args - 1, 1));
	objref ref = object;
	unsigned char temperature = ARG(args - 2, 1);

	if (ref.valid())
		Npc_setTemperature(&ref, temperature);
}

/* 0xbb: how warmly an NPC is dressed */
void far UC_GetNpcWarmth(Value *args, Value *ret)
{
	int object = GetItemRef(GetListNode(args - 1, 1));
	objref ref = object;

	if (ref.valid()) {
		int warmth = GetClothingWarmth(ref);
		ret->appendInt(warmth);
	} else
		ret->appendInt(0);
}

/* 0xbc: an NPC's identity number */
void far UC_GetNPCID(Value *args, Value *ret)
{
	int object = GetItemRef(GetListNode(args - 1, 1));
	objref ref = object;

	if (ref.valid()) {
		unsigned char id = Npc_getMagic(&ref);
		ret->appendInt(id);
	} else
		ret->appendInt(0);
}

/* 0xbd: set an NPC's identity number; the avatar keeps its own */
void far UC_SetNPCID(Value *args, Value *)
{
	int object = GetItemRef(GetListNode(args - 1, 1));
	objref ref = object;
	unsigned char id = ARG(args - 2, 1);

	if ((char)Item_isAvatar(&ref))
		return;
	if (ref.valid())
		Npc_setMagic(&ref, id);
}

/* 0xa6: the avatar's skin colour */
void far UC_AvatarSkin(Value *, Value *ret)
{
	ret->appendInt(Npc_getSkinColor(&AvatarRef));
}

/* 0x52: the weather */
void far UC_GetWeather(Value *args, Value *ret)
{
	ret->appendInt(CurrentWeather.type);
}

/* 0x53: change the weather */
void far UC_SetWeather(Value *args, Value *ret)
{
	SetWeather(&CurrentWeather, ARG(args - 1, 1));
}

/* 0x56: show one of the maps and wait for a click */
void far UC_DisplayMap(Value *args, Value *ret)
{
	unsigned char map = ARG(args - 1, 1);

	ShowWorldMap(map);
	ret->appendInt(0);
}
