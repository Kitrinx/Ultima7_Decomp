/* Black Gate U7.EXE, overlay segment 317 (file offsets 0x092960 to 0x0930e3, 1923 bytes).
 * Borland C++ 2.0 -mm -O -P rebuilds it byte for byte as C++.
 * Folder inferred from Serpent Isle's link groups.
 */

#include "objref.h"
#include "lowlevel.h"
#include "activity.h"
#include "iteminfo.h"
#include "voolook.h"
#include "daze.h"
#include "debug.h"
#include "coord.h"
#include "u7npc.h"
#include "sprite.h"
#include "u7ibuf.h"
#include "ucvalue.h"
#include "uclist.h"
#include "actqueue.h"
#include "death.h"
#include "makemojo.h"
#include "script.h"
#include "item.h"
#include "npcref.h"

/* a script built for the action queue: its length byte, then the actions */
struct Script {
	unsigned char length;
	char data[127];
	Script() { length = 1; }
};

/* runs a usable: the number follows as a word */
#define SCRIPT_USABLE   85

int SpeechTrack;

#define IS_NULL(v) ((unsigned char) ((v) == 0))

/* the int in element i of the usecode value at v */
#define ARG(v, i)   GetListNode(v, i)->toInt()

#define LORD_BRITISH_TYPE   466
#define BATLIN_TYPE         482

/* Usecode engine calls: args - 1 is the first argument, ret takes the result. */

/* 0x53: play a sprite effect at a position */
void UC_SpriteEffect(Value *args)
{
	int type = ARG(args - 1, 1);
	Coord x = ARG(args - 2, 1);
	Coord y = ARG(args - 3, 1);
	int dx = ARG(args - 4, 1);
	int dy = ARG(args - 5, 1);
	char z = ARG(args - 6, 1);
	int count = ARG(args - 7, 1);

	SpriteManager_playSprite(&gSpriteManager, x, y, dx, dy, type + 1024, z, count, 5);
}

/* 0x7b: play a sprite effect on an item */
void UC_ObjSpriteEffect(Value *args)
{
	int obj = GetItemRef(GetListNode(args - 1, 1));
	int type = ARG(args - 2, 1);
	int x = ARG(args - 3, 1);
	int y = ARG(args - 4, 1);
	int dx = ARG(args - 5, 1);
	int dy = ARG(args - 6, 1);
	char z = ARG(args - 7, 1);
	int count = ARG(args - 8, 1);

	SpriteManager_playSpriteForItem(&gSpriteManager, obj, x, y, dx, dy, type + 1024, z, count, 5);
}

/* 0x41: set an NPC to attack an item, or the position in the first argument when the item is 0 */
void far UC_SetToAttack(Value *args, Value *ret)
{
	int obj = GetItemRef(GetListNode(args - 1, 1));
	objref r = obj;
	int target = GetItemRef(GetListNode(args - 2, 1));
	int weapon;
	unsigned type = ARG(args - 3, 1);

	CheckMojoBounds(1024L, (unsigned long) type);
	weapon = PeekWord(WeaponLookup.addr + type * 2);
	if (!IS_NULL(weapon)) {
		ActionQueue.remove(obj, 1, 0);
		if (target != 0)
			Npc_setItemTarget(&r, target);
		else {
			Coord x = ARG(args - 1, 2);
			Coord y = ARG(args - 1, 3);
			int z = ARG(args - 1, 4);

			Npc_setCoordTarget(&r, x, y, z);
		}
		Npc_setTargetWeapon(&r, weapon);
		ret->appendInt(1);
	} else {
		CheatPrintfWait("Weapon is useless.");
		ret->appendInt(0);
	}
}

/* 0x3f: set an NPC waiting and send it to lunch */
void UC_RemoveNpc(Value *args)
{
	int obj = GetItemRef(GetListNode(args - 1, 1));
	objref r = obj;

	Npc_setSchedule(&r, WORK_WAIT);
	SendNPCToLunch(r);
}

/* 0x49 */
void UC_KillNpc(Value *args)
{
	int obj = GetItemRef(GetListNode(args - 1, 1));
	objref r = obj;

	KillNpc(r, 0);
}

/* 0x4d: copy an NPC; the copy is conjured, out of the party, and takes up the original's schedule */
void UC_Clone(Value *args)
{
	int obj = GetItemRef(GetListNode(args - 1, 1));
	objref r = obj;

	obj = SplitCreature(r);
	objref copy = obj;

	if (copy.valid()) {
		Item_setTemporary(&copy);
		ChangeStatus(&copy, NPC_IN_PARTY, 0);
		GetNpcBufferForIbo(&copy)->typeFlagsHigh = GetNpcBufferForIbo(&copy)->typeFlagsHigh | 0x40;
		Npc_setSchedule(&copy, GetNpcBufferForIbo(&r)->workType);
		GetNpcBufferForIbo(&copy)->schedules[GetNpcBufferForIbo(&copy)->currentSchedule].state = 1;
	}
}

/* 0x51: queue a script that brings a body's NPC back to life */
void UC_Resurrect(Value *args, Value *ret)
{
	int obj = GetItemRef(GetListNode(args - 1, 1));
	objref r = obj;

	if (r.valid() && GetBodyNpc(r)) {
		Script s;

		ActionQueue.remove(r.off, 1, 0);
		AppendScriptByte(&s.length, SCRIPT_FINISH);
		AppendScriptByte(&s.length, SCRIPT_NO_HALT);
		AppendScriptByte(&s.length, SCRIPT_USABLE);
		AppendScriptWord(&s.length, 30000);
		ActionQueue.add(r.off, (char *)&s);
		ret->appendInt(1);
	} else
		ret->appendInt(0);
}

/* 0x42: an item's z */
void UC_GetLift(Value *args, Value *ret)
{
	int obj = GetItemRef(GetListNode(args - 1, 1));
	objref r = obj;

	ret->appendInt(Item_getZ(&r));
}

/* 0x43: move an item to a new z */
void UC_SetLift(Value *args)
{
	int obj = GetItemRef(GetListNode(args - 1, 1));
	objref r = obj;
	unsigned char z = ARG(args - 2, 1);
	Coord x = Item_getX(r);
	Coord y = Item_getY(r);

	Item_move(&r, x, y, z);
}

void UC_RetiredEmptyCall(void) {}

/* 0x69 */
void UC_GetSpeechTrack(Value *args, Value *ret) { ret->appendInt(SpeechTrack); }

/* 0x5b: every NPC but Lord British and Batlin falls dead */
void UC_Armageddon(void)
{
	objref r;
	int i, type;

	for (i = 1; i <= NPC_COUNT; i++) {
		GetNpcIbo(&r, i);
		type = ITEM(r.off)->typeFrame & 0x3ff;
		if (type != LORD_BRITISH_TYPE && type != BATLIN_TYPE) {
			Item_setFrame(&r, 13);
			Item_setHitPoints(&r, (unsigned char) -10);
			ChangeStatus(&r, 0, NPC_DEAD);
		}
	}
	ArmageddonDone = 1;
}
