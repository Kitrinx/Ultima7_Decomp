/* Black Gate U7.EXE, overlay segment 259 (file offsets 0x07ae10 to 0x07b0db, 715 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Original folder unknown.
 */

#include "lowlevel.h"
#include "u7npc.h"
#include "item.h"
#include "npcref.h"
#include "voolook.h"
#include "actqueue.h"
#include "makemojo.h"
#include "monsters.h"
#include "script.h"
#include "special.h"
#include "search.h"

#define TYPE(rec) ((rec)->typeFrame & 0x3ff)
#define FRAME(rec) (((rec)->typeFrame & 0x7c00) >> 10)
#define IS_ASLEEP(n) ((char) (((n)->status & NPC_ASLEEP) != 0))

/* Whether the item shows its lying-down frame. */
char far IsInSleepFrame(ItemId item)
{
	objref ref = item.off;
	unsigned type = TYPE(ref.ptr());

	switch (type) {
	case 529:   /* slime */
		return 0;
	}
	return (FRAME(ref.ptr()) & 0xf) == 13;
}

/* Queues the NPC's lie-down animation, after its queued actions are removed when dying. Unless
 * dying, an asleep NPC is left alone. */
void far LayDown(objref npc, unsigned char dying)
{
	if (!dying && IS_ASLEEP(GetNpcBufferForIbo(&NPCRef(npc))))
		return;

	unsigned unusedRotation;
	unsigned char buf[128];

	buf[0] = 1;
	ActionQueue.remove(npc, 1, 0);
	if (dying)
		AppendScriptByte(buf, SCRIPT_REMOVE);
	unusedRotation = FRAME(ITEM(npc.off)) & ~0xf;
	switch (TYPE(ITEM(npc.off))) {
	case 529:   /* slime */
		AppendScriptByte(buf, 2);
		break;
	default:
		AppendScriptByte(buf, SCRIPT_STAND_FRAME);
		AppendScriptByte(buf, SCRIPT_KNEEL_FRAME);
		AppendScriptByte(buf, SCRIPT_SFX);
		AppendScriptByte(buf, 86);     /* the sound */
		AppendScriptByte(buf, SCRIPT_LIE_FRAME);
		break;
	}
	ActionQueue.add(npc, (char *)buf);
}

/* Reads the type's monster record, then returns the type unchanged. */
unsigned far GetStrangeMoverShape(unsigned far *typeFrame)
{
	MonsterRecord monster;
	AreaSearch unusedSearch;
	int index;
	unsigned type;

	type = *typeFrame & 0x3ff;
	CheckMojoBounds(1024L, (unsigned long) type);
	index = PeekWord(MonsterLookup.addr + type * 2);
	MonsterRecords.read(index, &monster);
	return *typeFrame & 0x3ff;
}

/* An item's hit points; some chests, doors and powder kegs with none still get a few. */
char far GetEffectiveHitPoints(ItemId id)
{
	objref ref = id.off;
	char hitPoints;
	unsigned frame;
	unsigned char quality;

	hitPoints = (char)Item_getHitPoints(&ref);
	if (hitPoints)
		return hitPoints;
	frame = FRAME(ref.ptr());
	quality = (unsigned char)Item_getQuality(&ref);
	switch (TYPE(ref.ptr())) {
	case 522:   /* locked chest */
		if (quality == 0 || quality == 255)
			return 4;
		break;
	case 270:   /* door */
	case 376:   /* door */
		if (quality == 0 && (frame < 3 || frame >= 8 && frame <= 10 || frame >= 16 && frame <= 18))
			return 6;
		break;
	case 432:   /* door */
	case 433:   /* door */
		if (quality == 0 && (frame < 3 || frame >= 4 && frame <= 6))
			return 6;
		break;
	case 704:   /* powder keg */
		if (quality == 0)
			return 4;
		break;
	}
	return 0;
}
