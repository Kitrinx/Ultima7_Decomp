/* Black Gate U7.EXE, overlay segment 329 (file offsets 0x099ca0 to 0x09a801, 2913 bytes).
 * Borland C++ 2.0 -mm -O -P rebuilds it byte for byte as C++.
 * Folder inferred from Serpent Isle's link groups.
 */

#include "u7port.h"
#include "iteminfo.h"
#include "itemrec.h"
#include "ucvalue.h"
#include "uclist.h"
#include "cast.h"
#include "random.h"
#include "u7npc.h"
#include "u7event.h"
#include "search.h"
#include "sounds.h"
#include "inter.h"
#include "type.h"
#include "objref.h"
#include "coord.h"
#include "item.h"
#include "target.h"
#include "gumpmgr.h"
#include "partymov.h"

extern "C" void PlaySfx(uint8_t number, int16_t volume, int16_t pan, int16_t flags);
extern objref AvatarRef;

/* the int in element i of the usecode value at v */
#define ARG(v, i)   GetListNode(v, i)->toInt()

/* the item in element 1 of the usecode value at v */
#define ITEM_ARG(v) GetItemRef(GetListNode(v, 1))

/* Usecode engine calls: args - 1 is the first argument, ret takes the result. */

/* 0x11: an item's type */
void UC_GetItemShape(Value *args, Value *ret)
{
	int16_t item = ITEM_ARG(args - 1);
	objref ref = item;

	ret->appendInt(ref.type());
}

/* 0x12 */
void UC_GetItemFrame(Value *args, Value *ret)
{
	int16_t item = ITEM_ARG(args - 1);
	objref ref = item;

	ret->appendInt(ref.frame());
}

/* 0x13: set an item's frame and flip bit */
void UC_SetItemFrame(Value *args)
{
	int16_t item = ITEM_ARG(args - 1);
	objref ref = item;

	ref.ptr()->asTypeFrame().setFlipFrame(ARG(args - 2, 1));
}

/* 0x14 */
void UC_GetQuality(Value *args, Value *ret)
{
	int16_t item = ITEM_ARG(args - 1);
	objref ref = item;

	ret->appendInt(Item_getQuality(&ref));
}

/* 0x15: set an item's quality; the result says whether it has one */
void UC_SetQuality(Value *args, Value *ret)
{
	int16_t item = ITEM_ARG(args - 1);
	objref ref = item;

	Item_setQuality(&ref, ARG(args - 2, 1));
	ret->appendInt(Item_hasQuality(&ref));
}

/* 0x16 */
void UC_GetItemQuantity(Value *args, Value *ret)
{
	int16_t item = ITEM_ARG(args - 1);
	objref ref = item;

	ret->appendInt(Item_getQuantity(&ref));
}

/* 0x17: set an item's quantity; the result says whether it has one */
void UC_SetQuantity(Value *args, Value *ret)
{
	int16_t item = ITEM_ARG(args - 1);
	objref ref = item;

	Item_setQuantity(ref, ARG(args - 2, 1), 0);
	ret->appendInt(Item_hasQuantity(&ref));
}

/* 0x18: an item's x, y and z */
void UC_GetCoord(Value *args, Value *ret)
{
	int16_t item = ITEM_ARG(args - 1);
	objref ref = item;

	int16_t x = Item_getX(ref);
	int16_t y = Item_getY(ref);
	int16_t z = Item_getZ(&ref);
	ret->appendInt(x);
	ret->appendInt(y);
	ret->appendInt(z);
}

/* 0x31 */
void UC_IsNpc(Value *args, Value *ret)
{
	int16_t item = ITEM_ARG(args - 1);
	objref ref = item;

	ret->appendInt((uint8_t)((ItemTypeClassFlags[gItemTypeInfo[ref.type()].typeClass] & CLASS_NPC) != 0));
}

/* 0x23: the party, last member first */
void UC_GetPartyList(Value *, Value *ret)
{
	int16_t i;

	for (i = PartySize - 1; i >= 0; i--)
		ret->appendInt(PartyMembers[i]);
}

/* 0x8d: the party, then its members who are down */
void UC_GetPartyList2(Value *, Value *ret)
{
	int16_t i;

	for (i = PartySize - 1; i >= 0; i--)
		ret->appendInt(PartyMembers[i]);
	for (i = DownedPartyCount - 1; i >= 0; i--)
		ret->appendInt(DownedPartyMembers[i]);
}

/* 0x6e */
void UC_GetContainer(Value *args, Value *ret)
{
	int16_t item = ITEM_ARG(args - 1);
	objref ref = item;

	ret->appendInt(Item_getContainer(&ref));
}

/* 0x6f */
void UC_RemoveItem(Value *args)
{
	int16_t item = ITEM_ARG(args - 1);
	objref ref = item;

	Item_delete(&ref);
}

/* 0x7f: an item says the text in its gump */
void UC_ItemSay2(Value *args)
{
	int16_t item = ITEM_ARG(args - 1);
	objref ref = item;

	ShowBarkInDialogs(ref, GetListNode(args - 2, 1)->text.str);
}

/* 0x7e */
void UC_CloseGumps()
{
	CloseDialogs();
}

/* 0x81 */
void UC_InGumpMode(Value *, Value *ret)
{
	ret->appendInt(InGumpMode());
}

/* 0x00: a number from 1 to the argument */
void UC_Random(Value *args, Value *ret)
{
	int16_t maximum = ARG(args - 1, 1);

	ret->appendInt(GenerateRandomIntegerInRange(maximum) + 1);
}

/* 0x10: a number from low to high */
void UC_DieRoll(Value *args, Value *ret)
{
	int16_t low = ARG(args - 1, 1);
	int16_t high = ARG(args - 2, 1);

	ret->appendInt(GenerateRandomIntegerInRange(high - low + 1) + low);
}

/* 0x5e */
void UC_GetArraySize(Value *args, Value *ret)
{
	ret->appendInt(LinkList_count(args - 1));
}

/* 0x3a: an NPC's usecode number: UC_AVATAR or its negated NPC number */
void UC_GetNpcNumber(Value *args, Value *ret)
{
	int16_t item = ITEM_ARG(args - 1);
	objref ref = item;

	if (ref == AvatarRef)
		ret->appendInt(UC_AVATAR);
	else
		ret->appendInt(-Item_getNpcNumber(&ref));
}

/* 0x35: the items of a type within radius of an item or position */
void UC_FindNearby(Value *args, Value *ret)
{
	int16_t type = ARG(args - 2, 1);
	int16_t radius = ARG(args - 3, 1);
	int16_t flags = ARG(args - 4, 1);
	AreaSearch items;
	int16_t count = LinkList_count(args - 1);
	Coord x, y;

	if (type == UC_ALL)
		type = -1;
	switch (count) {
	case 1: {
		int16_t item = ITEM_ARG(args - 1);
		x = Item_getX(objref(item));
		y = Item_getY(objref(item));
		FindItemInArea(&items, x - radius, y - radius, x + radius, y + radius,
			flags, type, 255, 255);
		break;
	}
	case 2:
	case 3:
		x = ARG(args - 1, 1);
		y = ARG(args - 1, 2);
		FindItemInArea(&items, x - radius, y - radius, x + radius, y + radius,
			flags, type, 255, 255);
		break;
	case 4:
		x = ARG(args - 1, 2);
		y = ARG(args - 1, 3);
		FindItemInArea(&items, x - radius, y - radius, x + radius, y + radius,
			flags, type, 255, 255);
		break;
	case 5: {
		x = ARG(args - 1, 1);
		y = ARG(args - 1, 2);
		int16_t quality = ARG(args - 1, 4);
		int16_t frame = ARG(args - 1, 5);
		if (quality == UC_ALL)
			quality = 255;
		if (frame == UC_ALL)
			frame = 255;
		FindItemInArea(&items, x - radius, y - radius, x + radius, y + radius,
			flags, type, quality, frame);
		break;
	}
	default:
		return;
	}
	while (items.found()) {
		ret->appendInt(items.current.off);
		FindItem(&items);
	}
}

/* 0x33: the item and position the player picks, or the avatar's combat target */
void UC_GetTarget(Value *, Value *ret)
{
	Coord x, y;
	int16_t z;
	objref item;

	if (CurrentEvent.id == EVENT_COMBAT) {
		if (Npc_hasItemTarget(&AvatarRef)) {
			int16_t selected = Npc_getItemTarget(&AvatarRef);
			item = objref(selected);
		} else {
			item.off = 0;
		}
		Npc_getTargetCoords(&AvatarRef, &x.value, &y.value, &z);
	} else {
		HavePlayerSelect(&item, &x, &y, &z);
	}
	ret->appendInt(item.off);
	ret->appendInt(x);
	ret->appendInt(y);
	ret->appendInt(z);
}

/* 0x22 */
void UC_GetAvatarRef(Value *, Value *ret)
{
	ret->appendInt(AvatarRef.off);
}

/* 0x0f: play a sound effect */
void UC_Sfx(Value *args)
{
	int16_t sound = ARG(args - 1, 1);

	PlaySfx(sound, 255, 64, 0);
}

/* 0x86: play a sound effect at an item */
void UC_PlaySoundEffect2(Value *args)
{
	int16_t sound = ARG(args - 1, 1);
	int16_t item = ITEM_ARG(args - 2);
	objref ref = item;

	PlaySoundAtItem(sound, ref);
}
