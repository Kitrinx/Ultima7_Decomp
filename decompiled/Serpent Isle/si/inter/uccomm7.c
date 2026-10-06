/* Serpent Isle SI.EXE, overlay segment 308 (file offsets 0x0866e0 to 0x086ff1, 2321 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include <stdio.h>
#include "itemrec.h"
#include "objref.h"
#include "ucvalue.h"
#include "uclist.h"
#include "type.h"
#include "item.h"
#include "coord.h"
#include "u7npc.h"
#include "u7event.h"
#include "debug.h"
#include "sprite.h"
#include "barge.h"
#include "camera.h"
#include "palctrl.h"
#include "party.h"
#include "search.h"
#include "wihh.h"
#include "legalmov.h"
#include "mapview.h"
#include "partymov.h"
#include "midiplay.h"
#include "usehook.h"
#include "u7sound.h"

extern objref AvatarRef;
int InfravisionEnabled = 0;

/* the int in element i of the usecode value at v */
#define ARG(v, i)   GetListNode(v, i)->toInt()

/* the item in element 1 of the usecode value at v */
#define ITEM_ARG(v) GetItemRef(GetListNode(v, 1))

#define TYPE_CLASS(ref) (gItemTypeInfo[(ref).type()].typeClass)

#define MYSTERIOUS_CRAFT_TYPE   267
#define BRITANNIA_TYPE          765     /* the orrery's centre */
#define PLANET_TYPE             988
#define MEDALLION_TYPE          955     /* frame 1 is the Fellowship's */

/* an NPC's alignment, in status bits 3 and 4 */
#define NPC_ALIGNMENT   0x18

/* Usecode engine calls: args - 1 is the first argument, ret takes the result. */

/* 0x82: an item was lit or put out; mark whether its holder carries light */
void far UC_SetLight(Value *args)
{
	int lights = 0;
	int object = ITEM_ARG(args - 1);
	objref ref = object;
	unsigned char lit = ARG(args - 2, 1);
	AreaSearch items;
	objref owner;

	owner = objref(Item_getContainer(&ref).off);
	if (owner.valid())
		FindItemInContainer(&items, owner, 0x100, -1, 255, 255);
	if ((unsigned char)(TYPE_CLASS(owner) == TYPE_CLASS_HUMAN) && owner.valid()) {
		while (items.current.valid()) {
			if ((unsigned char)gItemTypeInfo[items.current.objref::type()].light)
				lights++;
			FindItem(&items);
		}
		if (!lit && lights > 0)
			lights--;
		if (lights != 0)
			Item_setCarriesLight(&owner);
		else
			Item_clearCarriesLight(&owner);
	}
}

void far UC_SetInfravision(Value *args)
{
	int object = ARG(args - 1, 1);
	int enabled = ARG(args - 2, 1);

	if (enabled)
		InfravisionEnabled = 1;
	else
		InfravisionEnabled = 0;
}

/* 0x6d: whether the whole party stands on the avatar's barge */
void far UC_OnBarge(Value *, Value *ret)
{
	objref barge = FindBargeUnder(AvatarRef);

	if (!barge.valid()) {
		ret->appendInt(0);
		return;
	}
	for (int i = PartySize - 1; i >= 0; i--) {
		if (!(FindBargeUnder(PartyMembers[i]) == barge)) {
			ret->appendInt(0);
			return;
		}
	}
	ret->appendInt(1);
}

/* 0x2d */
void far UC_GetMusicTrack(Value *, Value *ret)
{
	ret->appendInt(CurrentMusic);
}

/* 0x2e: play a track, with a sprite over the item given */
void far UC_PlayMusic(Value *args)
{
	int track = ARG(args - 1, 1);
	int object = ITEM_ARG(args - 2);

	PlayMusic(track);
	objref ref;
	ref = objref(object);
	if (ref.valid())
		SpriteManager_playSpriteForItem(&gSpriteManager, object, 0, 0, -2, -2, 1048, 0, -1, 5);
}

/* 0x63: lay out the orrery's planets for a phase */
void far UC_SetOrrery(Value *args)
{
}

/* 0x62: whether a roof is overhead */
void far UC_IsPcInside(Value *, Value *ret)
{
	ret->appendInt(CeilingZ != 15);
}

/* 0x64 */
void far UC_SetPaletteFadedIn()
{
	MarkPaletteFadedIn();
	MarkPaletteFadedIn();
}

/* 0x67: whether the avatar wears the Fellowship medallion */
void far UC_WearingFellowship(Value *, Value *ret)
{
	objref ref = GetItemInSlot(AvatarRef, 4);

	ret->appendInt(ref.type() == MEDALLION_TYPE && ref.frame() == 1);
}

/* 0x68 */
void far UC_MouseExists(Value *, Value *ret)
{
	ret->appendInt(MouseExists());
}

/* 0x90: whether the terrain at a position is water */
void far UC_IsWater(Value *args, Value *ret)
{
	Coord x = ARG(args - 1, 1);
	Coord y = ARG(args - 1, 2);
	int dx = x - CellWindowX;
	int dy = y - CellWindowY;
	unsigned type = CellBuffer[dy][dx] & 0x3ff;

	ret->appendInt((int)(unsigned char)gItemTypeInfo[type].water != 0);
}

/* 0x95: the usable the next double-click runs */
void far UC_Telekinesis(Value *args)
{
	TelekinesisUsable = ARG(args - 1, 1);
}

void far PrintUsecodeNode(Node *node)
{
	int (far *print)(const char *, ...) = printf;
	switch (node->type) {
	case NODE_CHAR:
		print("'%c',\r\n", (char)node->integer());
		break;
	case NODE_INT:
		print("%d,\r\n", node->number);
		break;
	case NODE_TEXT:
		print("\"%s\",\r\n", node->text.str);
		break;
	default:
		print("Uninitialized\r\n");
	}
}

void far PrintUsecodeList(Value *value)
{
	int count = LinkList_count(value);

	for (int i = 1; i <= count; i++)
		PrintUsecodeNode(GetListNode(value, i));
}

/* 0x4e */
void far UC_DoNothing()
{
}

/* 0x6b: an item's frame with its flip bit */
void far UC_GetItemFrameRot(Value *args, Value *ret)
{
	int object = ITEM_ARG(args - 1);
	objref ref = object;

	ret->appendInt((ref.ptr()->typeFrame & 0xfc00) >> 10);
}

/* 0x6c */
void far UC_SetItemFrameRot(Value *args)
{
	int object = ITEM_ARG(args - 1);
	objref ref = object;

	ref.ptr()->asTypeFrame().setFlipFrame(ARG(args - 2, 1));
}

/* 0x3c: an NPC's alignment */
void far UC_GetAlignment(Value *args, Value *ret)
{
	int object = ITEM_ARG(args - 1);
	objref ref = object;

	if (ref.valid())
		ret->appendInt((unsigned char)((GetNpcBufferForIbo(&ref)->status & NPC_ALIGNMENT) >> 3));
	else
		ret->appendInt(0);
}

/* 0x3d */
void far UC_SetAlignment(Value *args)
{
	int object = ITEM_ARG(args - 1);
	objref ref = object;
	if (ref.valid()) {
		NpcBuffer far *actor = GetNpcBufferForIbo(&ref);
		actor->status &= ~NPC_ALIGNMENT;
		actor->status |= ((unsigned char)ARG(args - 2, 1) << 3) & NPC_ALIGNMENT;
	}
}

/* 0x3e: move an item, or the whole party for UC_PARTY */
void far UC_MoveObject(Value *args)
{
	int object = ITEM_ARG(args - 1);
	objref ref = object;
	if (ref.valid() && !IsDead(&ref)) {
		Coord x = ARG(args - 2, 1);
		Coord y = ARG(args - 2, 2);
		Coord z = ARG(args - 2, 3);

		if (object == UC_PARTY) {
			TeleportParty(x, y, z);
		} else {
			Item_move(&ref, x, y, z);
		}
		CenterOnAvatar();
	}
}

/* 0x85: whether a type and frame fit at a position */
void far UC_IsNotBlocked(Value *args, Value *ret)
{
	Coord x = ARG(args - 1, 1);
	Coord y = ARG(args - 1, 2);
	int z = ARG(args - 1, 3);
	unsigned type = ARG(args - 2, 1);
	unsigned frame = ARG(args - 3, 1);
	TypeFrame appearance(type, frame);

	ret->appendInt(CanTypeMoveTo(x, y, z, appearance.bits));
}
