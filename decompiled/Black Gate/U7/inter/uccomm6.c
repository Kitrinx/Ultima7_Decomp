/* Black Gate U7.EXE, overlay segment 327 (file offsets 0x098a80 to 0x098fcc, 1356 bytes).
 * Borland C++ 2.0 -mm -O -P rebuilds it byte for byte as C++.
 * Folder inferred from Serpent Isle's link groups.
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

#define ITEM(off) ((struct ItemRecord far *) MK_FP(ItemBufferSegment, (off)))
#define TYPE(rec) ((rec)->typeFrame & 0x3ff)
#define IS_VALID(r) ((char) ((r) != 0))

/* containers whose contents the party cannot give up */
#define LOCKED_CHEST_TYPE   522
#define SEALED_BOX_TYPE     798

extern objref AvatarRef;

extern objref far Item_getContainer(objref *);
extern unsigned char far Item_getQuantity(objref *ref);
extern char far Item_delete(objref *ref);

/* the int in element i of the usecode value at v */
#define ARG(v, i)   GetListNode(v, i)->toInt()

/* Usecode engine calls: args - 1 is the first argument, ret takes the result. */

/* 0x05: offer the answers in the first argument */
void far UC_TurnOn(Value *args, Value *)
{
	Node *node = 0;

	while (LinkList_stepForward(args - 1, (Link **)&node))
		OfferedAnswers.add(node->text.str);
}

/* 0x06: withdraw the answers in the first argument */
void far UC_TurnOff(Value *args, Value *)
{
	Node *node = 0;

	while (LinkList_stepForward(args - 1, (Link **)&node))
		OfferedAnswers.remove(node->text.str);
}

/* 0x07: save the answers offered */
void far UC_PushKeys(Value *, Value *)
{
	OfferedAnswers.push();
}

/* 0x08: bring back the answers saved */
void far UC_PopKeys(Value *, Value *)
{
	OfferedAnswers.pop();
}

/* 0x09: withdraw the answers offered since the last push */
void far UC_ClearKeys(Value *, Value *)
{
	OfferedAnswers.truncate();
}

/* 0x1e: add an NPC to the party */
void far UC_JoinParty(Value *args, Value *)
{
	int obj = GetItemRef(GetListNode(args - 1, 1));
	objref r = obj;

	AddToParty(r, 0);
}

/* 0x1f: take an NPC out of the party */
void far UC_LeaveParty(Value *args, Value *)
{
	int obj = GetItemRef(GetListNode(args - 1, 1));
	objref r = obj;

	RemoveFromParty(r, 1);
}

/* 0x27: the names of the NPCs in the first argument; other items are named by type */
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

/* 0x2b: take items from the party, member by member; the 1 asks the count for the whole party, so
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
	if (left < count)
		ret->appendInt(0);
	else {
		int n;

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
	got = GiveItemsToParty(count, type, quality, frame, temporary);
	if (got == 0)
		ret->appendInt(0);
	for (i = PartySize - 1; i >= 0; i--)
		if (got & (1 << i))
			ret->appendInt(PartyMembers[i]);
}

/* 0x5a: whether the avatar is female */
void far UC_AvatarSex(Value *, Value *ret)
{
	ret->appendInt(!Npc_isMale(&AvatarRef));
}

/* 0x1b: the item for an NPC number */
void far UC_GetNpcObject(Value *args, Value *ret)
{
	int obj = GetItemRef(GetListNode(args - 1, 1));

	ret->appendInt(obj);
}
