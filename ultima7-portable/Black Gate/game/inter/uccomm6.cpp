/* Black Gate U7.EXE, overlay segment 327 (file offsets 0x098a80 to 0x098fcc, 1356 bytes).
 * Borland C++ 2.0 -mm -O -P rebuilds it byte for byte as C++.
 * Folder inferred from Serpent Isle's link groups.
 */

#include "u7port.h"
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

#define ITEM(off) ((struct ItemRecord *) ItemAt((off)))
#define TYPE(rec) ((rec)->typeFrame & 0x3ff)
#define IS_VALID(r) ((int8_t) ((r) != 0))

/* containers whose contents the party cannot give up */
#define LOCKED_CHEST_TYPE   522
#define SEALED_BOX_TYPE     798

extern objref AvatarRef;

extern objref Item_getContainer(objref *);
extern uint8_t Item_getQuantity(objref *ref);
extern int8_t Item_delete(objref *ref);

/* the int in element i of the usecode value at v */
#define ARG(v, i)   GetListNode(v, i)->toInt()

/* Usecode engine calls: args - 1 is the first argument, ret takes the result. */

/* 0x05: offer the answers in the first argument */
void UC_TurnOn(Value *args, Value *)
{
	Node *node = 0;

	while (LinkList_stepForward(args - 1, (Link **)&node))
		OfferedAnswers.add(node->text.str);
}

/* 0x06: withdraw the answers in the first argument */
void UC_TurnOff(Value *args, Value *)
{
	Node *node = 0;

	while (LinkList_stepForward(args - 1, (Link **)&node))
		OfferedAnswers.remove(node->text.str);
}

/* 0x07: save the answers offered */
void UC_PushKeys(Value *, Value *)
{
	OfferedAnswers.push();
}

/* 0x08: bring back the answers saved */
void UC_PopKeys(Value *, Value *)
{
	OfferedAnswers.pop();
}

/* 0x09: withdraw the answers offered since the last push */
void UC_ClearKeys(Value *, Value *)
{
	OfferedAnswers.truncate();
}

/* 0x1e: add an NPC to the party */
void UC_JoinParty(Value *args, Value *)
{
	int16_t obj = GetItemRef(GetListNode(args - 1, 1));
	objref r = obj;

	AddToParty(r, 0);
}

/* 0x1f: take an NPC out of the party */
void UC_LeaveParty(Value *args, Value *)
{
	int16_t obj = GetItemRef(GetListNode(args - 1, 1));
	objref r = obj;

	RemoveFromParty(r, 1);
}

/* 0x27: the names of the NPCs in the first argument; other items are named by type */
void UC_GetNPCName(Value *args, Value *ret)
{
	Node *node = 0;
	int16_t obj;
	objref r;
	char buf[16];

	while (LinkList_stepForward(args - 1, (Link **)&node)) {
		obj = GetItemRef(node);
		r = obj;
		if ((uint16_t)Item_getNpcNumber(&r) < 256)
			ret->appendFarString(GetNpcBufferForIbo(&r)->name);
		else {
			sprintf(buf, "Type0x%x", TYPE(ITEM(r.off)));
			ret->appendFarString(buf);
		}
	}
}

/* 0x2b: take items from the party, member by member; the 1 asks the count for the whole party, so
 * who is never set */
void UC_RemovePartyItems(Value *args, Value *ret)
{
	int16_t count = ARG(args - 1, 1);
	int16_t type = ARG(args - 2, 1);
	int16_t quality = ARG(args - 3, 1);
	int16_t frame = ARG(args - 4, 1);
	int16_t left = 0;
	AreaSearch iter;
	objref who;
	int16_t i;

	if (quality == UC_ALL)
		quality = 255;
	if (frame == UC_ALL)
		frame = 255;
	left = CountHeldItems(1, who, type, quality, frame);
	if (left < count)
		ret->appendInt(0);
	else {
		int16_t n;

		left = count;
		n = PartySize;
		for (i = 0; i < n; i++) {
			FindItemInContainer(&iter, PartyMembers[i], 0, type, quality, frame);
			while (iter.found() && left) {
				if (IS_VALID(Item_getContainer(&iter.current).off)
					&& (TYPE(ITEM(Item_getContainer(&iter.current).off)) == LOCKED_CHEST_TYPE
					|| TYPE(ITEM(Item_getContainer(&iter.current).off)) == SEALED_BOX_TYPE))
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

/* 0x2c: give the party items; the result lists the members who took some */
void UC_AddPartyItems(Value *args, Value *ret)
{
	int16_t count = ARG(args - 1, 1);
	int16_t type = ARG(args - 2, 1);
	int16_t quality = ARG(args - 3, 1);
	int16_t frame = ARG(args - 4, 1);
	int16_t temporary = ARG(args - 5, 1);
	int16_t got;
	int16_t i;

	if (frame == UC_ALL)
		frame = 0;
	if (count <= 0)
		ret->appendInt(1);
	if (quality == UC_ALL)
		quality = 0;
	if (temporary == UC_ALL)
		temporary = 1;
	got = GiveItemsToParty(count, type, quality, frame, temporary);
	if (got == 0)
		ret->appendInt(0);
	for (i = PartySize - 1; i >= 0; i--)
		if (got & (1 << i))
			ret->appendInt(PartyMembers[i]);
}

/* 0x5a: whether the avatar is female */
void UC_AvatarSex(Value *, Value *ret)
{
	ret->appendInt(!Npc_isMale(&AvatarRef));
}

/* 0x1b: the item for an NPC number */
void UC_GetNpcObject(Value *args, Value *ret)
{
	int16_t obj = GetItemRef(GetListNode(args - 1, 1));

	ret->appendInt(obj);
}
