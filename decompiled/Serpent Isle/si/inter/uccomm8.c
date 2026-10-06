/* Serpent Isle SI.EXE, overlay segment 309 (file offsets 0x0870a0 to 0x087d44, 3236 bytes).
 * Borland C++ 2.0 -mm -O -P rebuilds it byte for byte as C++.
 */

#include "iteminfo.h"
#include "itemrec.h"
#include "ucvalue.h"
#include "uclist.h"
#include "cast.h"
#include "random.h"
#include "npcref.h"
#include "u7npc.h"
#include "u7event.h"
#include "search.h"
#include "sounds.h"
#include "u7sound.h"
#include "inter.h"
#include "type.h"
#include "objref.h"
#include "coord.h"
#include "item.h"
#include "target.h"
#include "gumpmgr.h"
#include "partymov.h"
#include "weight.h"

extern objref AvatarRef;
extern void far SendNPCToLunch(NPCRef npc);

#define USECODE_CONTAINER_TYPE  486
#define AVATAR_TYPE             721

#define IS_CLASS(r, c) ((char)(gItemTypeInfo[(r).type()].typeClass == (c)))
#define INFO_FLAG(v, bit) ((unsigned char)((v).flags & (bit)))

/* the int in element i of the usecode value at v */
#define ARG(v, i)   GetListNode(v, i)->toInt()

/* the item in element 1 of the usecode value at v */
#define ITEM_ARG(v) GetItemRef(GetListNode(v, 1))

/* Usecode engine calls: args - 1 is the first argument, ret takes the result. */

/* 0x18: an item's type */
void far UC_GetItemShape(Value *args, Value *ret)
{
	int item = ITEM_ARG(args - 1);
	objref ref = item;

	ret->appendInt(ref.type());
}

/* 0x1a */
void far UC_GetItemFrame(Value *args, Value *ret)
{
	int item = ITEM_ARG(args - 1);
	objref ref = item;

	ret->appendInt(ref.frame());
}

/* 0x19: the weight of an item's type */
void far UC_GetItemWeight(Value *args, Value *ret)
{
	int item = ITEM_ARG(args - 1);
	objref ref = item;

	ret->appendInt(GetItemTypeWeight(ref.type()));
}

/* 0x1b: set an item's frame and flip bit */
void far UC_SetItemFrame(Value *args)
{
	int item = ITEM_ARG(args - 1);
	objref ref = item;

	ref.ptr()->asTypeFrame().setFlipFrame(ARG(args - 2, 1));
}

/* 0x1c */
void far UC_GetQuality(Value *args, Value *ret)
{
	int item = ITEM_ARG(args - 1);
	objref ref = item;

	ret->appendInt(Item_getQuality(&ref));
}

/* 0x1d: set an item's quality; the result says whether it has one */
void far UC_SetQuality(Value *args, Value *ret)
{
	int item = ITEM_ARG(args - 1);
	objref ref = item;

	Item_setQuality(&ref, ARG(args - 2, 1));
	ret->appendInt(Item_hasQuality(&ref));
}

/* 0x1e */
void far UC_GetItemQuantity(Value *args, Value *ret)
{
	int item = ITEM_ARG(args - 1);
	objref ref = item;

	ret->appendInt(Item_getQuantity(&ref));
}

/* 0x1f: set an item's quantity; the result says whether it has one */
void far UC_SetQuantity(Value *args, Value *ret)
{
	int item = ITEM_ARG(args - 1);
	objref ref = item;

	Item_setQuantity(ref, ARG(args - 2, 1), 0);
	ret->appendInt(Item_hasQuantity(&ref));
}

/* 0x20: an item's x, y and z */
void far UC_GetCoord(Value *args, Value *ret)
{
	int item = ITEM_ARG(args - 1);
	objref ref = item;

	int x = Item_getX(ref);
	int y = Item_getY(ref);
	int z = Item_getZ(&ref);
	ret->appendInt(x);
	ret->appendInt(y);
	ret->appendInt(z);
}

/* 0x3d */
void far UC_IsNpc(Value *args, Value *ret)
{
	int item = ITEM_ARG(args - 1);
	objref ref = item;

	ret->appendInt((unsigned char)((ItemTypeClassFlags[gItemTypeInfo[ref.type()].typeClass] & CLASS_NPC) != 0));
}

/* 0x2b: the party, last member first */
void far UC_GetPartyList(Value *, Value *ret)
{
	int i;

	for (i = PartySize - 1; i >= 0; i--)
		ret->appendInt(PartyMembers[i]);
}

/* 0xaa: the party, then its members who are down */
void far UC_GetPartyList2(Value *, Value *ret)
{
	int i;

	for (i = PartySize - 1; i >= 0; i--)
		ret->appendInt(PartyMembers[i]);
	for (i = DownedPartyCount - 1; i >= 0; i--)
		ret->appendInt(DownedPartyMembers[i]);
}

/* 0x83 */
void far UC_GetContainer(Value *args, Value *ret)
{
	int item = ITEM_ARG(args - 1);
	objref ref = item;

	ret->appendInt(Item_getContainer(&ref));
}

/* 0x84: delete an item; a human, or a monster that is not temporary, is sent off the map instead,
 * and the usecode container, anything in it and the avatar are left alone */
void far UC_RemoveItem(Value *args)
{
	int item = ITEM_ARG(args - 1);
	objref ref = item;

	if (ref.valid()) {
		if (Item_getContainer(&ref).type() == USECODE_CONTAINER_TYPE || ref.type() == USECODE_CONTAINER_TYPE
			|| ref.type() == AVATAR_TYPE)
			return;
		if (IS_CLASS(ref, TYPE_CLASS_HUMAN)
			|| IS_CLASS(ref, TYPE_CLASS_MONSTER) && !INFO_FLAG(GetItemZAndStuff(&ref), 8))
			SendNPCToLunch(NPCRef(ref));
		else
			Item_delete(&ref);
	}
}

/* 0x99: an item says the text in its gump */
void far UC_ItemSay2(Value *args)
{
	int item = ITEM_ARG(args - 1);
	objref ref = item;

	ShowBarkInDialogs(ref, GetListNode(args - 2, 1)->text.str);
}

/* 0x98 */
void far UC_CloseGumps()
{
	CloseDialogs();
}

/* 0x9b */
void far UC_InGumpMode(Value *, Value *ret)
{
	ret->appendInt(InGumpMode());
}

/* 0x00: a number from 1 to the argument */
void far UC_Random(Value *args, Value *ret)
{
	int maximum = ARG(args - 1, 1);

	ret->appendInt(GenerateRandomIntegerInRange(maximum) + 1);
}

/* 0x17: a number from low to high */
void far UC_DieRoll(Value *args, Value *ret)
{
	int low = ARG(args - 1, 1);
	int high = ARG(args - 2, 1);

	ret->appendInt(GenerateRandomIntegerInRange(high - low + 1) + low);
}

/* 0x73 */
void far UC_GetArraySize(Value *args, Value *ret)
{
	ret->appendInt(LinkList_count(args - 1));
}

/* 0x47: an NPC's usecode number: UC_AVATAR or its negated NPC number, 0 for no item */
void far UC_GetNpcNumber(Value *args, Value *ret)
{
	int item = ITEM_ARG(args - 1);
	objref ref = item;

	if (ref == AvatarRef)
		ret->appendInt(UC_AVATAR);
	else if ((char)ref.valid())
		ret->appendInt(-Item_getNpcNumber(&ref));
	else
		ret->appendInt(0);
}

/* 0x41: the items of a type within radius of an item or position */
void far UC_FindNearby(Value *args, Value *ret)
{
	int type = ARG(args - 2, 1);
	int radius = ARG(args - 3, 1);
	int flags = ARG(args - 4, 1);
	AreaSearch items;
	int count = LinkList_count(args - 1);
	Coord x, y;

	if (type == UC_ALL)
		type = -1;
	switch (count) {
	case 1:
		int item = ITEM_ARG(args - 1);
		x = Item_getX(objref(item));
		y = Item_getY(objref(item));
		FindItemInArea(&items, x - radius, y - radius, x + radius, y + radius,
			flags, type, 255, 255);
		break;
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
	case 5:
		x = ARG(args - 1, 1);
		y = ARG(args - 1, 2);
		int quality = ARG(args - 1, 4);
		int frame = ARG(args - 1, 5);
		if (quality == UC_ALL)
			quality = 255;
		if (frame == UC_ALL)
			frame = 255;
		FindItemInArea(&items, x - radius, y - radius, x + radius, y + radius,
			flags, type, quality, frame);
		break;
	default:
		return;
	}
	while (items.found()) {
		ret->appendInt(items.current.off);
		FindItem(&items);
	}
}

/* 0x3f: the item and position the player picks, or the avatar's combat target */
void far UC_GetTarget(Value *, Value *ret)
{
	Coord x, y;
	int z;
	objref item;

	if (CurrentEvent.id == EVENT_COMBAT) {
		if (Npc_hasItemTarget(&AvatarRef)) {
			int selected = Npc_getItemTarget(&AvatarRef);
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

/* 0x2a */
void far UC_GetAvatarRef(Value *, Value *ret)
{
	ret->appendInt(AvatarRef.off);
}

/* 0x16: play a sound effect */
void far UC_Sfx(Value *args)
{
	int sound = ARG(args - 1, 1);

	PlaySfx(sound, 255, 64);
}

/* 0xa1: play a sound effect at an item */
void far UC_PlaySoundEffect2(Value *args)
{
	int sound = ARG(args - 1, 1);
	int item = ITEM_ARG(args - 2);
	objref ref = item;

	PlaySoundAtItem(sound, ref);
}
