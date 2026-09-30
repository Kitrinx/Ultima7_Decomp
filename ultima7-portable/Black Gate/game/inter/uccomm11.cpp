/* Black Gate U7.EXE, overlay segment 332 (file offsets 0x09b7d0 to 0x09ba23, 595 bytes).
 * Borland C++ 2.0 -mm -O -P rebuilds it byte for byte as C++.
 * Folder inferred from Serpent Isle's link groups.
 */

#include "u7port.h"
#include "plat.h"
#include "u7event.h"
#include <string.h>
#include "ucvalue.h"
#include "uclist.h"
#include "u7npc.h"
#include "weather.h"
#include "item.h"
#include "npcref.h"
#include "tools.h"
#include "uccomm7.h"
#include "colbuf.h"
#include "gumps.h"
#include "debug.h"
#include "convmgr.h"
#include "combmode.h"

/* the int in element i of the usecode value at v */
#define ARG(v, i)   GetListNode(v, i)->toInt()

#define NPC(p)  GetNpcBufferForIbo(p)
#define TYPE(rec)   ((rec)->typeFrame & 0x3ff)

#define SIGN_TEXT_SIZE  60

/* Usecode engine calls: args - 1 is the first argument, ret takes the result. */

/* 0x32: show the lines of text in the second argument, 59 characters at most */
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

/* 0x44: the weather */
void UC_GetWeather(Value *args, Value *ret)
{
	ret->appendInt(CurrentWeather.type);
}

/* 0x45: change the weather */
void UC_SetWeather(Value *args, Value *ret)
{
	SetWeather(&CurrentWeather, ARG(args - 1, 1));
}

/* 0x48: show the map of Britannia */
void UC_DisplayMap(Value *args, Value *ret)
{
	ShowWorldMap();
	ret->appendInt(0);
}

/* 0x34: print free memory and the argument, then wait for a key */
void UC_ErrorMessage(Value *args, Value *ret)
{
	DebugPrintf("core = %u", 0);   /* no free-memory figure on the host */
	PrintUsecodeList(args - 1);
	while (!KeyPressed())
		plat_yield();
	ReadKey();
}

/* 0x55: open a book or scroll of an item's type */
void UC_BookMode(Value *args, Value *ret)
{
	int16_t obj = GetItemRef(GetListNode(args - 1, 1));
	int16_t type = TYPE(objref(obj).ptr());

	BeginConversation(type);
}

/* 0x4b: set an NPC's attack mode */
void UC_SetAttackMode(Value *args, Value *ret)
{
	int16_t obj = GetItemRef(GetListNode(args - 1, 1));
	NPCRef r;
	r.off = obj;
	int16_t mode = ARG(args - 2, 1);

	SetAttackMode(r, mode);
}

/* 0x4c: store the second object's NPC number in the first NPC */
void UC_SetOppressor(Value *args, Value *ret)
{
	int16_t obj = GetItemRef(GetListNode(args - 1, 1));
	objref r = obj;
	int16_t other = GetItemRef(GetListNode(args - 2, 1));
	objref o = other;

	if (obj != other)
		NPC(&r)->oppressor = Item_getNpcNumber(&o);
}
