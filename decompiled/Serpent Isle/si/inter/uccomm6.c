/* Serpent Isle SI.EXE, overlay segment 307 (file offsets 0x085980 to 0x0865d3, 3155 bytes).
 * Borland C++ 2.0 -mm -O -P rebuilds it byte for byte as C++.
 */

#include <stdio.h>
#include "itemrec.h"
#include "ucvalue.h"
#include "uclist.h"
#include "u7npc.h"
#include "partymov.h"
#include "keywords.h"
#include "bogus.h"
#include "cast.h"
#include "party.h"
#include "search.h"
#include "item.h"
#include "itemrec.h"
#include "coord.h"
#include "mapview.h"
#include "dosio.h"
#include "typefram.h"
#include "type.h"
#include "u7manage.h"

unsigned char far BringNpcNearAvatar(objref member, int unused, int limit);

#define ITEM(off) ((struct ItemRecord far *) MK_FP(ItemBufferSegment, (off)))
#define TYPE(rec) ((rec)->typeFrame & 0x3ff)
#define IS_VALID(r) ((char) ((r) != 0))

/* a container whose contents the party cannot give up */
#define LOCKED_CHEST_TYPE   522

extern objref AvatarRef;

extern objref far Item_getContainer(objref *);
extern unsigned char far Item_getQuantity(objref *ref);
extern char far Item_delete(objref *ref);

/* the int in element i of the usecode value at v */
#define ARG(v, i)   GetListNode(v, i)->toInt()

/* Usecode engine calls: args - 1 is the first argument, ret takes the result. */

/* 0x0c: offer the answers in the first argument */
void far UC_TurnOn(Value *args, Value *)
{
	Node *node = 0;

	while (LinkList_stepForward(args - 1, (Link **)&node))
		OfferedAnswers.add(node->text.str);
}

/* 0x0d: withdraw the answers in the first argument */
void far UC_TurnOff(Value *args, Value *)
{
	Node *node = 0;

	while (LinkList_stepForward(args - 1, (Link **)&node))
		OfferedAnswers.remove(node->text.str);
}

/* 0x0e: save the answers offered */
void far UC_PushKeys(Value *, Value *)
{
	OfferedAnswers.push();
}

/* 0x0f: bring back the answers saved */
void far UC_PopKeys(Value *, Value *)
{
	OfferedAnswers.pop();
}

/* 0x10: withdraw the answers offered since the last push */
void far UC_ClearKeys(Value *, Value *)
{
	OfferedAnswers.truncate();
}

/* 0x26: add an NPC to the party */
void far UC_JoinParty(Value *args, Value *)
{
	int obj = GetItemRef(GetListNode(args - 1, 1));
	objref r = obj;

	if (r.valid())
		AddToParty(r, 0);
}

/* 0x27: take an NPC out of the party */
void far UC_LeaveParty(Value *args, Value *)
{
	int obj = GetItemRef(GetListNode(args - 1, 1));
	objref r = obj;

	if (r.valid())
		RemoveFromParty(r, 1);
}

/* 0x30: the names of the NPCs in the first argument; other items are named by type */
void far UC_GetNPCName(Value *args, Value *ret)
{
	Node *node = 0;
	int obj;
	objref r;
	char buf[16];

	while (LinkList_stepForward(args - 1, (Link **)&node)) {
		obj = GetItemRef(node);
		r = obj;
		if ((unsigned)Item_getNpcNumber(&r) < 256)
			ret->appendFarString(GetNpcBufferForIbo(&r)->name);
		else {
			sprintf(buf, "Type0x%x", TYPE(ITEM(r.off)));
			ret->appendFarString(buf);
		}
	}
}

/* 0x34: take items from the party, member by member; the 1 asks the count for the whole party, so
 * who is never set */
void far UC_RemovePartyItems(Value *args, Value *ret)
{
	int count = ARG(args - 1, 1);
	int type = ARG(args - 2, 1);
	int quality = ARG(args - 3, 1);
	int frame = ARG(args - 4, 1);
	int left = 0;
	AreaSearch iter;
	objref who;
	int i;

	if (quality == UC_ALL)
		quality = 255;
	if (frame == UC_ALL)
		frame = 255;
	left = CountHeldItems(1, who, type, quality, frame);
	if (count == UC_ALL)
		count = left;
	if (left < count || left == 0)
		ret->appendInt(0);
	else {
		int n;

		left = count;
		n = PartySize;
		for (i = 0; i < n; i++) {
			FindItemInContainer(&iter, PartyMembers[i], 0, type, quality, frame);
			while (iter.found() && left) {
				if (IS_VALID(Item_getContainer(&iter.current).off)
					&& TYPE(ITEM(Item_getContainer(&iter.current).off)) == LOCKED_CHEST_TYPE)
					iter.current.next();    /* fetched and dropped: the search does not move */
				else if (Item_getQuantity(&iter.current) <= left) {
					left -= Item_getQuantity(&iter.current);
					Item_delete(&iter.current);
					FindItemInContainer(&iter, PartyMembers[i], 0, type, quality, frame);
				} else {
					Item_setQuantity(iter.current, Item_getQuantity(&iter.current) - left, 0);
					left = 0;
				}
			}
			if (left == 0)
				break;
		}
	}
	ret->appendInt(1);
}

/* 0x35: give the party items; the result lists the members who took some, then the count left
 * over, which is placed at the avatar's feet in stacks of 100 */
void far UC_AddPartyItems(Value *args, Value *ret)
{
	int count = ARG(args - 1, 1);
	int type = ARG(args - 2, 1);
	int quality = ARG(args - 3, 1);
	int frame = ARG(args - 4, 1);
	int temporary = ARG(args - 5, 1);
	int got;
	int i;

	if (frame == UC_ALL)
		frame = 0;
	if (count <= 0)
		ret->appendInt(1);
	if (quality == UC_ALL)
		quality = 0;
	if (temporary == UC_ALL)
		temporary = 1;
	got = GiveItemsToParty(&count, type, quality, frame, temporary);
	if (got == 0)
		ret->appendInt(0);
	for (i = PartySize - 1; i >= 0; i--)
		if (got & (1 << i))
			ret->appendInt(PartyMembers[i]);
	ret->appendInt(count);
	if (count == 0)
		return;

	Coord x = Item_getX(AvatarRef);
	Coord y = Item_getY(AvatarRef);
	int z = Item_getZ(&AvatarRef);
	TypeFrame shape = 0;
	shape.setType(type);
	shape.setFrame(ClampShapeFrame(type, frame));
	for (; count > 0; count -= 100) {
		objref r;
		char n = count > 100 ? 100 : count;
		CreateItem(&r, shape);
		Item_setQuality(&r, quality);
		Item_setQuantity(r, n, 1);
		if (temporary)
			Item_setTemporary(&r);
		Item_setOkayToTake(&r);

		int placed = PlaceItem(&r, x, y, z);
		if (!placed && !IsTemporary(&r)) {
			AreaSearch s;

			FindItemInArea(&s, CellWindowX, CellWindowY, Coord(CellWindowX + (CELL_WINDOW - 1)),
				Coord(CellWindowY + (CELL_WINDOW - 1)), 0, -1, 255, 255);
			while (s.found() && !IsTemporary(&s.current))
				FindItem(&s);
			if (!s.found() || !(unsigned char)Item_delete(&s.current) || !PlaceItem(&r, x, y, z)) {
				ReportErrorSubtype(0x6401, 1);
				ret->appendInt(0);
				return;
			}
			placed = 1;
		} else if (!placed)
			ZapDetachedItem(&r);
	}
}

/* 0x36: put items into a container; the result is the count that did not fit */
void far UC_GiveToCont(Value *args, Value *ret)
{
	int obj = GetItemRef(GetListNode(args - 1, 1));
	int count = ARG(args - 2, 1);
	int type = ARG(args - 3, 1);
	int quality = ARG(args - 4, 1);
	int frame = ARG(args - 5, 1);
	int temporary = ARG(args - 6, 1);

	if (frame == UC_ALL)
		frame = 0;
	if (count <= 0)
		ret->appendInt(1);
	if (quality == UC_ALL)
		quality = 0;
	if (temporary == UC_ALL)
		temporary = 1;
	objref cont = obj;
	objref r;

	if (!cont.valid()) {
		ret->appendInt(0);
		return;
	}
	TypeFrame shape = 0;
	shape.setType(type);
	shape.setFrame(ClampShapeFrame(type, frame));
	int n = 1;
	int given = 0;
	while (count) {
		if (gItemTypeInfo[shape.type()].typeClass == TYPE_CLASS_QUANTITY)
			n = count > 100 ? 100 : count;
		if (CreateItem(&r, shape)) {
			Item_setFrame(&r, frame);
			Item_setQuality(&r, quality);
			Item_setQuantity(r, n, 1);
			if (temporary)
				Item_setTemporary(&r);
			Item_setOkayToTake(&r);
		} else
			break;
		if (TryToPlaceItem(obj, 0, 1)) {
			ZapDetachedItem(&r);
			break;
		}
		count -= n;
		given += n;
	}
	ret->appendInt(count);
}

/* 0x37: take items out of a container, or out of the party for UC_PARTY */
void far UC_TakeFromCont(Value *args, Value *ret)
{
	int obj = GetItemRef(GetListNode(args - 1, 1));
	int count = ARG(args - 2, 1);
	int type = ARG(args - 3, 1);
	int quality = ARG(args - 4, 1);
	int frame = ARG(args - 5, 1);
	int left = 0;
	AreaSearch iter;
	objref cont = obj;

	if (!cont.valid()) {
		ret->appendInt(0);
		return;
	}
	if (quality == UC_ALL)
		quality = 255;
	if (frame == UC_ALL)
		frame = 255;
	if (obj == UC_PARTY) {
		left = CountHeldItems(1, cont, type, quality, frame);
	} else {
		left = CountHeldItems(0, cont, type, quality, frame);
	}
	if (count == UC_ALL)
		count = left;
	if (left < count || left == 0)
		ret->appendInt(0);
	else {
		left = count;
		FindItemInContainer(&iter, cont, 0, type, quality, frame);
		while (iter.found() && left) {
			if (Item_getQuantity(&iter.current) <= left) {
				left -= Item_getQuantity(&iter.current);
				Item_delete(&iter.current);
				FindItemInContainer(&iter, cont, 0, type, quality, frame);
			} else {
				Item_setQuantity(iter.current, Item_getQuantity(&iter.current) - left, 0);
				left = 0;
			}
		}
	}
	ret->appendInt(1);
}

/* 0x6f: whether the avatar is female */
void far UC_AvatarSex(Value *, Value *ret)
{
	ret->appendInt((int)Npc_isMale(&AvatarRef) ? 0 : 1);
}

/* 0x23: the item for an NPC number */
void far UC_GetNpcObject(Value *args, Value *ret)
{
	int obj = GetItemRef(GetListNode(args - 1, 1));

	ret->appendInt(obj);
}

/* 0xbf: bring an NPC to the avatar; the result says whether it came */
void far UC_ApproachAvatar(Value *args, Value *ret)
{
	int obj = GetItemRef(GetListNode(args - 1, 1));
	int unused = ARG(args - 2, 1);
	int limit = ARG(args - 3, 1);

	ret->appendInt(BringNpcNearAvatar(obj, unused, limit));
}
