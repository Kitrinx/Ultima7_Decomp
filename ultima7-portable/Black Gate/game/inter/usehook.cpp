/* Black Gate U7.EXE, overlay segment 315 (file offsets 0x091830 to 0x091bfb, 971 bytes).
 * Borland C++ 2.0 -mm -O -P -Y rebuilds it byte for byte as C++.
 * Folder inferred from Serpent Isle's link groups.
 */

#include "u7port.h"
#include "item.h"
#include "u7event.h"
#include "colbuf.h"
#include "debug.h"
#include "ucstack.h"
#include "routine.h"
#include "sprite.h"
#include "inter.h"
#include "npcref.h"
#include "tools.h"
#include "u7ibuf.h"
#include "memfree.h"
#include "keywords.h"
#include "ucctrl.h"
#include "type.h"

#define TYPE(rec) ((rec)->typeFrame & 0x3ff)
#define TYPE_CLASS(rec) (gItemTypeInfo[TYPE(rec)].typeClass)
#define CLASS_FLAGS(rec) (ItemTypeClassFlags[TYPE_CLASS(rec)])
#define IS_NPC(rec) ((int8_t) ((CLASS_FLAGS(rec) & CLASS_NPC) != 0))

/* usable numbers: NPCs 0 to 255 have theirs from NPC_USABLES on */
#define NPC_USABLES     0x400
#define NPC_USABLES_END 0x500
#define BOOK_USABLE     0x638
#define OINK_USABLE     0x63d

#define BOOK_TYPE       642

extern uint8_t SaveLoadActive;

int16_t TelekinesisUsable = -1;
int8_t UsableRunning = 0;

/* Run usecode function func for an item, or the item's own function when func is -1. */
int8_t RunUsable(uint8_t event, int16_t item, uint16_t func)
{
	if (SaveLoadActive)
		return 0;

	UsecodeRoutine code;
	ValueStack stack;
	CallStack calls;
	uint16_t mem;

	mem = NearMemory.getNearFree();
	if (mem < 3500)
		DebugPrintf("\rLow Mem!! %d", mem);
	if (mem < 3000)
		return 0;
	if (func == (uint16_t)-1) {
		if (IS_NPC(objref(item).ptr())) {
			if (event == EVENT_ACTION && IsNpcUnconscious(&objref(item))) {
				return 0;
			}
			if (GameInput.isGumpMode() && !(uint8_t)Item_isAvatar(&objref(item))) {
				return 0;
			}
			if (OinkMode && Item_getNpcNumber(&objref(item)) != 26)
				func = OINK_USABLE;
			else {
				if ((uint16_t)Item_getNpcNumber(&objref(item)) >= 256) {
					func = TYPE(objref(item).ptr());
				} else {
					func = Item_getNpcNumber(&objref(item)) + NPC_USABLES;
				}
			}
		} else {
			func = TYPE(objref(item).ptr());
		}
	}
	objref obj = item;
	if (TYPE(obj.ptr()) == BOOK_TYPE && (uint8_t)Item_getQuality(&obj) >= 100)
		func = BOOK_USABLE;
	if (func == (uint16_t)TelekinesisUsable)
		event = EVENT_ACTION;
	/* one function at a time */
	if (UsableRunning) {
		return 0;
	}
	UsableRunning = 1;
	if (!code.load(func)) {
		UsableRunning = 0;
		return 0;
	}
	SpriteManager_stopItemSprites(&gSpriteManager, item, 1);
	UC_Interpret(&code, &stack, &calls, ChosenAnswer, item, event);
	if (func > NPC_USABLES && func < NPC_USABLES_END && event == EVENT_ACTION)
		CheckItemBuffer();
	if (func == (uint16_t)TelekinesisUsable)
		TelekinesisUsable = -1;
	UsableRunning = 0;
	return 1;
}

/* Whether usecode function func exists. */
uint8_t HasUsable(uint16_t func)
{
	UsecodeRoutine code;

	return code.load(func);
}
