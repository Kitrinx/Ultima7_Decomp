/* Serpent Isle SI.EXE, overlay segment 310 (file offsets 0x087e50 to 0x0882a4, 1108 bytes).
 * Borland C++ 2.0 -mm -O -P rebuilds it byte for byte as C++.
 */

#include "u7event.h"
#include "item.h"
#include "ucvalue.h"
#include "uclist.h"
#include "sprite.h"
#include "gumpmgr.h"
#include "script.h"
#include "type.h"
#include "damage.h"
#include "actqueue.h"

#define TYPE(rec) ((rec)->typeFrame & 0x3ff)
#define TYPE_CLASS(rec) (gItemTypeInfo[TYPE(rec)].typeClass)
#define CLASS_FLAGS(rec) (ItemTypeClassFlags[TYPE_CLASS(rec)])
#define IS_NPC(rec) ((char) ((CLASS_FLAGS(rec) & CLASS_NPC) != 0))

extern int far IsActionQueueRoom();

/* the int in element i of the usecode value at v */
#define ARG(v, i)   GetListNode(v, i)->toInt()

/* the text of element i of the usecode value at v */
#define TEXT(v, i)  (GetListNode(v, i)->text.str)

/* Usecode engine calls: args - 1 is the first argument, ret takes the result. */

/* 0x4d: an item says the text */
void far UC_ItemSay(Value *args, Value *)
{
	int obj = GetItemRef(GetListNode(args - 1, 1));

	if (InGumpMode()) {
		objref r = obj;

		ShowBarkInDialogs(r, TEXT(args - 2, 1));
	} else
		SpriteManager_barkOnItem(&gSpriteManager, obj, TEXT(args - 2, 1), 0, 15, 0);
}

/* 0x4e: the item stops talking */
void far UC_ClearBark(Value *args, Value *)
{
	int obj = GetItemRef(GetListNode(args - 1, 1));

	ClearAllBarks(obj);
	ActionQueue.suppressBarks(obj);
}

/* 0x01: queue the script in the second argument for an item; strings go in quoted, numbers above 255
 * as two bytes. Without SCRIPT_NO_HALT the item's other scripts are removed first. */
void far UC_Post(Value *args, Value *ret)
{
	Node *node = 0;
	int v;
	int result = 0;
	int ended = 0;
	int obj;
	objref r;
	char c;
	unsigned char buf[128];
	char *s;

	buf[0] = 1;
	obj = GetItemRef(GetListNode(args - 1, 1));
	while (LinkList_stepForward(args - 2, (Link **)&node)) {
		if (node->type == NODE_TEXT) {
			s = node->text.str;
			AppendScriptByte(buf, '"');
			while (*s) {
				AppendScriptByte(buf, *s);
				s++;
			}
			AppendScriptByte(buf, '"');
		} else {
			v = node->toInt();
			if (v > 255) {
				AppendScriptByte(buf, v & 0xff);
				AppendScriptByte(buf, v >> 8);
			} else {
				c = v;
				if (c == SCRIPT_NO_HALT)
					ended = 1;
				AppendScriptByte(buf, c);
			}
		}
	}
	if (!ended)
		ActionQueue.remove(obj, 1, 0);
	r = obj;
	if (IS_NPC(ITEM((int) r)) || (char)IsActionQueueRoom())
		result = ActionQueue.add(obj, (char *)buf);
	ret->appendInt(result);
}

/* 0x02: as 0x01, after the delay in the third argument */
void far UC_PostInFuture(Value *args, Value *ret)
{
	Node *node = 0;
	int v;
	int result = 0;
	int ended = 0;
	int obj;
	int delay;
	objref r;
	unsigned char buf[128];
	char *s;

	buf[0] = 1;
	obj = GetItemRef(GetListNode(args - 1, 1));
	delay = ARG(args - 3, 1) + 1;
	if (delay < 1)
		delay = 1;
	while (LinkList_stepForward(args - 2, (Link **)&node)) {
		v = node->toInt();
		if (node->type == NODE_TEXT) {
			s = node->text.str;
			AppendScriptByte(buf, '"');
			while (*s) {
				AppendScriptByte(buf, *s);
				s++;
			}
			AppendScriptByte(buf, '"');
		} else {
			v = node->toInt();
			if (v > 255) {
				AppendScriptByte(buf, v & 0xff);
				AppendScriptByte(buf, v >> 8);
			} else {
				if (v == SCRIPT_NO_HALT)
					ended = 1;
				AppendScriptByte(buf, v);
			}
		}
	}
	if (!ended)
		ActionQueue.remove(obj, 1, 0);
	r = obj;
	if (IS_NPC(ITEM((int) r)) || (char)IsActionQueueRoom())
		result = ActionQueue.add(delay, obj, (char *)buf);
	ret->appendInt(result);
}

/* 0x8f: whether the item has a script queued */
void far UC_InUsecode(Value *args, Value *ret)
{
	int obj = GetItemRef(GetListNode(args - 1, 1));

	ret->appendInt(ActionQueue.contains(obj));
}

/* 0x58: attack against defence */
void far UC_RollToWin(Value *args, Value *ret)
{
	char attack = ARG(args - 1, 1);
	char defence = ARG(args - 2, 1);

	ret->appendInt(RollToWin(attack, defence));
}
