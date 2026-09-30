/* Black Gate U7.EXE, overlay segment 319 (file offsets 0x093750 to 0x09408e, 2366 bytes).
 * Borland C++ 2.0 -mm -O -P -Y rebuilds it byte for byte as C++.
 * Original folder unknown.
 */

#include "u7port.h"
#include <new>
#include "iteminfo.h"
#include "objref.h"
#include "datanode.h"
#include "main.h"
#include "uccomm5.h"
#include "ucvalue.h"
#include "uclist.h"
#include "barge.h"
#include "palctrl.h"
#include "tools.h"
#include "u7event.h"
#include "u7ibuf.h"
#include "worldpal.h"
#include "coord.h"
#include "npcref.h"
#include "bogus.h"
#include "item.h"
#include "party.h"
#include "search.h"
#include "gtimer.h"
#include "camera.h"
#include "mapview.h"

#define USECODE_TIMER_COUNT 12

/* the int in element i of the usecode value at v */
#define ARG(v, i)   GetListNode(v, i)->toInt()

/* the item in element 1 of the usecode value at v */
#define ITEM_ARG(v) GetItemRef(GetListNode(v, 1))

/* the two body types */
#define BODY_TYPE       400
#define OTHER_BODY_TYPE 414

extern GameTimer UsecodeTimers[USECODE_TIMER_COUNT];
extern uint8_t DoesSpellbookHaveSpell(objref *book, uint8_t spell);
extern void AddSpellToSpellbook(objref *, uint8_t);

extern void CenterOnAvatar();

/* Usecode engine calls: args - 1 is the first argument, ret takes the result. */

/* 0x52: add a spell to a spellbook; the result says whether it was new */
void UC_AddSpell(Value *args, Value *ret)
{
	int16_t spell = ARG(args - 1, 1);
	int16_t mode = ARG(args - 2, 1);
	int16_t item = ITEM_ARG(args - 3);
	objref book = item;
	int16_t added = 0;

	if (!DoesSpellbookHaveSpell(&book, spell)) {
		AddSpellToSpellbook(&book, spell);
		added = 1;
	}
	ret->appendInt(added);
}

/* 0x57: light for the given time */
void UC_CauseLight(Value *args)
{
	int16_t value = ARG(args - 1, 1);

	GameScreen.lightSpellTime = value;
}

/* 0x56: stop time for the given time */
void UC_StopTime(Value *args)
{
	int16_t value = ARG(args - 1, 1);

	GameTime.hold = value;
}

/* 0x58: the barge under an item */
void UC_GetBarge(Value *args, Value *ret)
{
	int16_t item = ITEM_ARG(args - 1);
	objref object = item;

	ret->appendInt(FindBargeUnder(object));
}

/* 0x4f: show the world around a position until a click or key */
void UC_DisplayArea(Value *args)
{
	Coord x = ARG(args - 1, 1);
	Coord y = ARG(args - 1, 2);
	uint8_t z = Coord(ARG(args - 1, 3));

	RemoteViewActive = 1;
	Camera_drawAt(&gCamera, x, y, z);
	WaitForClickOrKey();
	RemoteViewActive = 0;
	CenterOnAvatar();
}

/* 0x50: look through the wizard eye */
void UC_WizardEye(Value *args)
{
	RunWizardEye(ARG(args - 1, 1));
}

/* 0x59: shake the screen */
void UC_Earthquake(Value *args)
{
	int16_t value = ARG(args - 1, 1);

	StartEarthquake(value);
}

/* 0x5d: flash lightning */
void UC_Lightning() { FlashLightning(); }

/* 0x38: the hour of the day */
void UC_GetHour(Value *args, Value *ret)
{
	ret->appendInt(GameTime.getHour());
}

/* 0x39: the minute of the hour */
void UC_GameMinute(Value *args, Value *ret)
{
	ret->appendInt(GameTime.getMinute());
}

/* 0x65: hours since usecode timer index was set */
void UC_GetClock(Value *args, Value *ret)
{
	int16_t index = ARG(args - 1, 1);
	int16_t now = GameTime.getHour() + GameTime.getDay() * 24;
	int16_t saved = UsecodeTimers[index].getHour() + UsecodeTimers[index].getDay() * 24;

	ret->appendInt(now - saved);
}

/* 0x66: set usecode timer index to now */
void UC_SetClock(Value *args)
{
	int16_t index = ARG(args - 1, 1);

	UsecodeTimers[index] = GameTime;
}

/* 0x5f: remember the avatar's position in a virtue stone, z in its quality */
void UC_MarkVirtueStone(Value *args)
{
	objref stone = ARG(args - 1, 1);

	stone.position(Item_getX(AvatarRef), Item_getY(AvatarRef));
	Item_setQuality(&stone, Item_getZ(&AvatarRef));
}

/* 0x60: take the party to where a virtue stone was marked */
void UC_RecallVirtueStone(Value *args)
{
	objref stone = ARG(args - 1, 1);

	TeleportParty(stone.worldX(), stone.worldY(), Item_getQuality(&stone));
}

/* 0x0e: the item of a type nearest an item; distance -1 searches 20 cells and needs the item on screen */
void UC_FindNearest(Value *args, Value *ret)
{
	uint8_t visible = 0;
	int16_t item = ITEM_ARG(args - 1);
	int16_t type = ARG(args - 2, 1);
	int16_t distance = ARG(args - 3, 1);

	if (type == UC_ALL)
		type = -1;
	Coord x = Item_getX(objref(item));
	Coord y = Item_getY(objref(item));
	if (distance == -1) {
		distance = 20;
		visible = 1;
	}
	AreaSearch items;
	FindNearestItem(&items, x, y, distance, 0, type, 255, 255);
	if (visible && !(uint8_t)gCamera.isOnScreen(x, y))
		ret->appendInt(0);
	else
		ret->appendInt(items.current.off);
}

/* 0x70: back to the arrow cursor */
void UC_RestoreMouseCursor() { SelectArrowCursor(); }

void UC_SelectNoArrowCursor() { SelectNoArrowCursor(); }

/* 0x73: restart the game */
void UC_RestartGame() { RestartRequested = 1; }

/* 0x75: end the game, with the ending when the argument is set */
void UC_RunEndgame(Value *args)
{
	if (ARG(args - 1, 1))
		EndgameRequested = 1;
	else
		EndgameQuitRequested = 1;
}

/* 0x92: point the camera at an item */
void UC_SetCamera(Value *args)
{
	int16_t item = ITEM_ARG(args - 1);
	objref object = item;

	gCamera.setTarget(object);
}

/* 0x93: the bodies of dead party members within 40 cells of an item */
void UC_GetDeadParty(Value *args, Value *ret)
{
	AreaSearch items;
	int16_t type;
	uint16_t npc;
	int16_t i;
	Coord left, right, top, bottom;
	uint8_t quality, frame;
	int16_t item = ITEM_ARG(args - 1);
	objref object = item;

	left = Item_getX(object) - Coord(40);
	right = Item_getX(object) + Coord(40);
	top = Item_getY(object) - Coord(40);
	bottom = Item_getY(object) + Coord(40);
	for (i = 0; i < DeadPartyCount; ++i) {
		/* a body's quality and frame hold its NPC's number */
		npc = Item_getNpcNumber((objref *)&NPCRef(DeadPartyMembers[i]));
		quality = npc & 0x7f;
		frame = (npc & 0x180) >> 7;
		for (int16_t kind = 0; kind <= 1; ++kind) {
			if (kind == 0)
				type = BODY_TYPE;
			else
				type = OTHER_BODY_TYPE;
			FindItemInArea(&items, left, top, right, bottom, 0, type, frame, 255);
			while (items.found()) {
				if (Item_getQuantity(&items.current) == quality) {
					ret->appendInt(items.current.off);
					break;
				}
				FindItem(&items);
			}
		}
	}
}

/* 0x94: centre the view on a position */
void UC_ViewTile(Value *args)
{
	Coord x = ARG(args - 1, 1);
	Coord y = ARG(args - 1, 2);

	MainWorldView.setCenter(x, y);
}

inline GameTimer::GameTimer() { init(43, 1); }
GameTimer UsecodeTimers[USECODE_TIMER_COUNT];

extern "C" void ResetUccomm4Globals(void)
{
	memset((void *)UsecodeTimers, 0, sizeof(UsecodeTimers));
}

extern "C" void ConstructUccomm4Globals(void)
{
	for (int i = 0; i < USECODE_TIMER_COUNT; i++)
		new (&UsecodeTimers[i]) GameTimer();
}
