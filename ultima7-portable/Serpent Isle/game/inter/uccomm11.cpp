/* Serpent Isle SI.EXE, overlay segment 312 (file offsets 0x089750 to 0x089ccb, 1403 bytes).
 * Borland C++ 2.0 -mm -O -P rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include "plat.h"
#include "u7event.h"
#include <string.h>
#include "ucvalue.h"
#include "uclist.h"
#include "u7npc.h"
#include "item.h"
#include "npcref.h"
#include "uccomm7.h"
#include "debug.h"
#include "combmode.h"
#include "combatai.h"
#include "equip.h"
#include "wihh.h"
#include "coord.h"
#include "typefram.h"

void ShowSign(int16_t shape, char *text);
void BeginConversation(int16_t type);

/* the int in element i of the usecode value at v */
#define ARG(v, i)   GetListNode(v, i)->toInt()

#define NPC(p)  GetNpcBufferForIbo(p)
#define TYPE(rec)   ((rec)->typeFrame & 0x3ff)

#define SIGN_TEXT_SIZE  60

/* Usecode engine calls: args - 1 is the first argument, ret takes the result. */

/* 0x3e: show the lines of text in the second argument, 59 characters at most */
void UC_DisplayRunes(Value *args, Value *ret)
{
	int16_t len;
	char *s;
	int16_t type;
	int16_t count;
	char buf[SIGN_TEXT_SIZE];
	int16_t pos = 0;
	int16_t i;

	type = ARG(args - 1, 1);
	memset(buf, 0, SIGN_TEXT_SIZE);
	count = LinkList_count(args - 2);
	for (i = 0; i < count; i++) {
		s = GetListNode(args - 2, i + 1)->text.str;
		len = strlen(s);
		strncpy(buf + pos, s, SIGN_TEXT_SIZE - 1 - pos);
		pos += len + 1;
		buf[pos - 1] = '\r';
		if (pos >= SIGN_TEXT_SIZE) {
			pos = SIGN_TEXT_SIZE - 1;
			break;
		}
	}
	buf[pos] = 0;
	ShowSign(type, buf);
}

/* 0x40: print free memory and the argument, then wait for a key */
void UC_ErrorMessage(Value *args, Value *ret)
{
	DebugPrintf("core = %u", 0);   /* no free-memory figure on the host */
	PrintUsecodeList(args - 1);
	while (!KeyPressed())
		plat_yield();
	ReadKey();
}

/* 0x6a: open a book or scroll of an item's type */
void UC_BookMode(Value *args, Value *ret)
{
	int16_t obj = GetItemRef(GetListNode(args - 1, 1));
	int16_t type = TYPE(objref(obj).ptr());

	BeginConversation(type);
}

/* 0x59: set an NPC's attack mode */
void UC_SetAttackMode(Value *args, Value *ret)
{
	int16_t obj = GetItemRef(GetListNode(args - 1, 1));
	NPCRef r;
	r.off = obj;
	int16_t mode = ARG(args - 2, 1);

	if (r.valid())
		SetAttackMode(r, mode);
}

/* 0x5a: an NPC's attack mode, read from its quality; 0 for no item */
void UC_GetAttackMode(Value *args, Value *ret)
{
	int16_t obj = GetItemRef(GetListNode(args - 1, 1));
	objref r = obj;

	if (r.valid())
		ret->appendInt(Item_getQuality(&r));
	else
		ret->appendInt(0);
}

/* 0x5b: store the second object's NPC number in the first NPC */
void UC_SetOppressor(Value *args, Value *ret)
{
	int16_t obj = GetItemRef(GetListNode(args - 1, 1));
	objref r = obj;
	int16_t other = GetItemRef(GetListNode(args - 2, 1));
	objref o = other;

	if (r.valid() && obj != other)
		NPC(&r)->oppressor = Item_getNpcNumber(&o);
}

/* 0x5c: the NPC number an NPC holds as its oppressor */
void UC_GetAttacker(Value *args, Value *ret)
{
	int16_t obj = GetItemRef(GetListNode(args - 1, 1));
	objref r = obj;

	if (r.valid())
		ret->appendInt(NPC(&r)->oppressor);
	else
		ret->appendInt(0);
}

/* 0x2d: make a creature of a type at x, y, z, equipped and waiting for its schedule */
void UC_CreateCreature(Value *args, Value *ret)
{
	objref r;
	int16_t type = ARG(args - 1, 1);
	int16_t created = CreateNpc(&r, TypeFrame(type), ARG(args - 2, 1), ARG(args - 2, 2), ARG(args - 2, 3));

	if (created) {
		RandomizeCreature(r, TypeFrame(r.ptr()->typeFrame));
		Npc_setSchedule(&r, 12);
		NPC(&r)->schedules[NPC(&r)->currentSchedule].state = 1;
		created = r.off;
	}
	ret->appendInt(created);
}

/* 0x5d: the type of the weapon in an NPC's hand, 0 for no item */
void UC_GetWeapon(Value *args, Value *ret)
{
	int16_t obj = GetItemRef(GetListNode(args - 1, 1));
	objref r = obj;

	if (r.valid()) {
		int16_t type = objref(GetItemInSlot(r, 1)).type();
		ret->appendInt(type);
	} else
		ret->appendInt(0);
}

/* 0x5e: set an NPC's target to another item and make it fight */
void UC_SetTarget(Value *args, Value *ret)
{
	NPCRef r;
	r.off = GetItemRef(GetListNode(args - 1, 1));
	objref o = GetItemRef(GetListNode(args - 2, 1));

	if (r.valid()) {
		NPC(&r)->typeFlagsHigh = NPC(&r)->typeFlagsHigh | 1;
		Npc_setTarget(&r, Item_getNpcNumber(&o), 1);
	}
}

/* 0xbe: the item in one of an NPC's equipment slots */
void UC_GetEquip(Value *args, Value *ret)
{
	int16_t obj = GetItemRef(GetListNode(args - 1, 1));
	uint8_t slot = ARG(args - 2, 1);

	ret->appendInt(objref(GetItemInSlot(objref(obj), slot)));
}
