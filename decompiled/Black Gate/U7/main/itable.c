/* Black Gate U7.EXE, resident segment 7 (file offsets 0x00e289 to 0x00f312, 4233 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "activity.h"
#include "view.h"
#include "itemrec.h"
#include "iteminfo.h"
#include "objref.h"
#include "coord.h"
#include "maps.h"
#include "u7npc.h"
#include "u7manage.h"
#include "sprite.h"
#include "actqueue.h"
#include "gtimer.h"
#include "itemcmd.h"
#include "random.h"
#include "collide.h"
#include "daze.h"
#include "text.h"
#include "u7point.h"
#include "script.h"
#include "type.h"
#include "npcref.h"
#include "bogus.h"
#include "damage.h"
#include "partymov.h"
#include "mapview.h"
#include "itable.h"

#define MK_FP(seg, off) ((void _seg *)(seg) + (void near *)(off))
typedef unsigned char boolean;

/* where a type class 11 item stands, kept in its extra record */
struct PositionRecord { unsigned char cellX, cellY, region; };

extern char far Item_getHitPoints(objref *ref);
extern void far Item_setHitPoints(objref *, unsigned char);
extern unsigned char far Item_getQualityFlags(objref *);

extern Coord far Item_getX(objref &);
extern Coord far Item_getY(objref &);
extern unsigned char far Item_move(objref *, Loc, Loc, int);

inline boolean HasNoHitPoints(objref *ref) { return Item_getHitPoints(ref) <= 0; }
inline boolean IsVeryHungry(objref *ref)
{
	return GetNpcBufferForIbo(ref)->food < 5 && GetNpcBufferForIbo(ref)->food > 0;
}
inline unsigned char GetItemFlags(objref *ref, unsigned char mask)
{
	return Item_getQualityFlags(ref) & mask;
}

unsigned char objref::cellX()
{
	unsigned char value = 0;

	if ((unsigned char)(gItemTypeInfo[ptr()->typeFrame & 0x3ff].typeClass == TYPE_CLASS_VIRTUE_STONE)) {
		int position = ptr()->data.extra;
		value = ((PositionRecord far *)MK_FP(ItemBufferSegment, position))->cellX;
	}
	return value;
}

void objref::cellX(int value)
{
	if ((unsigned char)(gItemTypeInfo[ptr()->typeFrame & 0x3ff].typeClass == TYPE_CLASS_VIRTUE_STONE)) {
		int position = ptr()->data.extra;
		((PositionRecord far *)MK_FP(ItemBufferSegment, position))->cellX = value;
	}
}

unsigned char objref::cellY()
{
	unsigned char value = 0;

	if ((unsigned char)(gItemTypeInfo[ptr()->typeFrame & 0x3ff].typeClass == TYPE_CLASS_VIRTUE_STONE)) {
		int position = ptr()->data.extra;
		value = ((PositionRecord far *)MK_FP(ItemBufferSegment, position))->cellY;
	}
	return value;
}

void objref::cellY(int value)
{
	if ((unsigned char)(gItemTypeInfo[ptr()->typeFrame & 0x3ff].typeClass == TYPE_CLASS_VIRTUE_STONE)) {
		int position = ptr()->data.extra;
		((PositionRecord far *)MK_FP(ItemBufferSegment, position))->cellY = value;
	}
}

unsigned char objref::region()
{
	unsigned char value = 0;

	if ((unsigned char)(gItemTypeInfo[ptr()->typeFrame & 0x3ff].typeClass == TYPE_CLASS_VIRTUE_STONE)) {
		int position = ptr()->data.extra;
		value = ((PositionRecord far *)MK_FP(ItemBufferSegment, position))->region;
	}
	return value;
}

void objref::region(unsigned char value)
{
	if ((unsigned char)(gItemTypeInfo[ptr()->typeFrame & 0x3ff].typeClass == TYPE_CLASS_VIRTUE_STONE)) {
		int position = ptr()->data.extra;
		((PositionRecord far *)MK_FP(ItemBufferSegment, position))->region = value;
	}
}

Coord objref::worldX()
{
	Coord result;

	result.value = cellX() + RegionX[region()].value;
	return result;
}

Coord objref::worldY()
{
	Coord result;

	result.value = cellY() + RegionY[region()].value;
	return result;
}

void objref::position(Coord x, Coord y)
{
	unsigned char area = GetRegionAt(x, y, CurrentMap);

	cellX(x.value & 0xff);
	cellY(y.value & 0xff);
	region(area);
}

TextPrinter::~TextPrinter()
{
}

TextPrinter::TextPrinter()
{
	target = &ScreenView;
	x = 0;
	y = 0;
	spacing = 0;
	leading = 0;
}

void TextPrinter::printChar(char c)
{
	drawChar(target, x, y, c);
	if (c == '\n') {
		y += charHeight(c) + leading;
		x = target->clip.x0;
	} else if (c == '\r') {
		x = target->clip.x0;
	} else {
		x += charWidth(c) + spacing;
	}
}

void TextPrinter::printString(char *text)
{
	for (; *text; text++)
		printChar(*text);
}

int TextPrinter::textWidth(char *text)
{
	int width = 0;

	for (; *text; text++) {
		width += charWidth(*text);
		width += spacing;
	}
	return width;
}

void UpdateNPCStatus()
{
	NPCRef ref;
	char health, strength, healed, increment;
	boolean recover, wasDead;
	unsigned char mana, maxMana;
	unsigned type;

	recover = GameTime.ticks % TICKS_PER_HOUR == 0;
	for (int number = 0; number < NPC_COUNT; number++) {
		GetNpcIbo(&ref, number);
		if (ref.valid()) {
			if (recover) {
				health = Item_getHitPoints(&ref);
				if (health <= 0 || !HasStatus(&ref, NPC_IN_PARTY) || IsWellFed(&ref)) {
					strength = GetNpcBufferForIbo(&ref)->strength;
					if (health < strength) {
						if (health <= 0) {
							wasDead = 1;
						} else
							wasDead = 0;
						increment = strength >> 4;
						if (increment == 0) {
							increment = 1;
						}
						healed = health + increment;
						Item_setHitPoints(&ref, healed > strength ? strength : healed);
						if (wasDead && !HasNoHitPoints(&ref)) {
							if (CanVisit(&ref) && !GetItemFlags(&ref, 0x20))
								EndSleep(ref);
							else {
								NpcBuffer far *record = GetNpcBufferForIbo(&ref);
								record->changeFlags(NPC_ASLEEP, 0);
							}
						}
					}
				}
				if ((unsigned char)(gItemTypeInfo[ref.type()].typeClass == TYPE_CLASS_HUMAN)) {
					mana = GetNpcBufferForIbo(&ref)->mana;
					maxMana = GetNpcBufferForIbo(&ref)->magic;
					if (mana < maxMana) {
						mana = (unsigned char)(mana + maxMana + 1) / 2;
						if (GetNpcBufferForIbo(&ref)->mana == mana) {
							++mana;
						}
						GetNpcBufferForIbo(&ref)->mana = mana;
					}
				}
				if (HasStatus(&ref, NPC_IN_PARTY) && GetNpcBufferForIbo(&ref)->food > 0) {
					--GetNpcBufferForIbo(&ref)->food;
				}
			}
			if (CanVisit(&ref)) {
				if (!GetItemFlags(&ref, 0x20) && Item_getHitPoints(&ref) <= 0 &&
					GenerateRandomIntegerInRange(1000) < 15) {
					Item_setHitPoints(&ref, Item_getHitPoints(&ref) + 1);
					if (Item_getHitPoints(&ref) >= 1) {
						EndSleep(ref);
					}
				}
				if (HasStatus(&ref, NPC_POISONED) && GenerateRandomIntegerInRange(10000) < 5) {
					EndPoison(ref);
				}
				if (HasStatus(&ref, NPC_POISONED) && GenerateRandomIntegerInRange(1000) < 5) {
					ReduceHealth(ref, 1, 3, 0);
				}
				if (!GetItemFlags(&ref, 0x20) && HasStatus(&ref, NPC_ASLEEP) &&
					Item_getHitPoints(&ref) > 0 && GetNpcBufferForIbo(&ref)->workType != WORK_SLEEP &&
					GenerateRandomIntegerInRange(1000) < 10) {
					EndSleep(ref);
				}
				if (GetItemFlags(&ref, 1)) {
					WearOffInvisibility(ref);
				}
				if (HasStatus(&ref, NPC_CHARMED) && GenerateRandomIntegerInRange(1000) < 5) {
					EndCharm(ref);
				}
				if (HasStatus(&ref, NPC_PARALYZED) && GenerateRandomIntegerInRange(1000) < 10) {
					EndParalysis(ref);
				}
				if (HasStatus(&ref, NPC_CURSED) && GenerateRandomIntegerInRange(1000) < 5) {
					EndCurse(ref);
				}
				if (HasStatus(&ref, NPC_PROTECTED) && GenerateRandomIntegerInRange(1000) < 5) {
					ChangeStatus(&ref, NPC_PROTECTED, 0);
				}
				if (GetNpcBufferForIbo(&ref)->marked(0x10) &&
					GenerateRandomIntegerInRange(1000) < 5) {
					GetNpcBufferForIbo(&ref)->typeFlagsHigh = GetNpcBufferForIbo(&ref)->typeFlagsHigh & ~0x10;
				}
				if (GetNpcBufferForIbo(&ref)->moving(0x10) &&
					!HasStatus(&ref, NPC_ASLEEP) && !HasStatus(&ref, NPC_PARALYZED) &&
					!GetItemFlags(&ref, 0x20) && GetNpcBufferForIbo(&ref)->workType != WORK_STAND &&
					!(unsigned char)HasNPCStepped(Item_getNpcNumber(&ref))) {
					ActionQueue.add(ref.off, (char *)MakeScript(SCRIPT_NO_HALT, 0x57, 1, 0xff, SCRIPT_END));
				}
				if (Item_getHitPoints(&ref) > 0 && GenerateRandomIntegerInRange(1000) < 100) {
					CheckContactEffects(ref);
				}
				if (number > 255 && GetNpcBufferForIbo(&ref)->marked(0x80) &&
					GenerateRandomIntegerInRange(1000) < 1) {
					GetNpcBufferForIbo(&ref)->typeFlagsHigh = GetNpcBufferForIbo(&ref)->typeFlagsHigh & ~0x40;
					GetNpcBufferForIbo(&ref)->typeFlagsHigh = GetNpcBufferForIbo(&ref)->typeFlagsHigh & ~0x80;
					type = ref.type();
					ChangeStatus(&ref, 0x18, 0x18);   /* alignment: chaotic */
					Npc_setSchedule(&ref, type == 354 || RollChance(4) ? 0 : 12);
					GetNpcBufferForIbo(&ref)->schedules[GetNpcBufferForIbo(&ref)->currentSchedule].state = 1;
				}
				if (HasStatus(&ref, NPC_IN_PARTY)) {
					if (GetNpcBufferForIbo(&ref)->workType == WORK_COMBAT) {
						if (IsFamished(&ref) && Item_getHitPoints(&ref) > 0
							&& GenerateRandomIntegerInRange(1000) < 15) {
							ReduceHealth(ref, 1, 3, 0);
						}
					} else if (IsHungry(&ref)) {
						if (GenerateRandomIntegerInRange(1000) < 4) {
							SpriteManager_barkOnItem(&gSpriteManager, ref.off,
								GetGameText(1, GenerateRandomIntegerInRange(3) + 119), 0, 15, 0);
						}
					} else if (IsVeryHungry(&ref)) {
						if (GenerateRandomIntegerInRange(1000) < 8) {
							SpriteManager_barkOnItem(&gSpriteManager, ref.off,
								GetGameText(1, GenerateRandomIntegerInRange(3) + 122), 0, 15, 0);
						}
					} else if (IsFamished(&ref)) {
						if (Item_getHitPoints(&ref) > 0 && GenerateRandomIntegerInRange(1000) < 15) {
							ReduceHealth(ref, 1, 3, 0);
						}
						if (GenerateRandomIntegerInRange(1000) < 15) {
							SpriteManager_barkOnItem(&gSpriteManager, ref.off,
								GetGameText(1, GenerateRandomIntegerInRange(3) + 125), 0, 15, 0);
						}
					}
				}
			}
		}
	}
}

void HidePointer()
{
	HideCursor();
}

void ShowPointer()
{
	ShowCursor();
}

void SetPointerZ(int value)
{
	AdjustCursorZ(value - CursorCenterZ);
}

void ShiftPointerZ(char value)
{
	AdjustCursorZ(value);
}

unsigned GetCursorLength()
{
	return ArrowLength;
}

void HideMarkerShapes()
{
	RemapShapeRecord(275, 688);
	RemapShapeRecord(200, 688);
	RemapShapeRecord(961, 688);
	RemapShapeRecord(607, 688);
}

unsigned char PostItemScript(unsigned char *script, objref ref)
{
	script[0] = 4;
	script[1] = 0x57;
	return ActionQueue.add(ref.off, (char *)script);
}

extern "C" void RunItemScript(unsigned char *script, objref ref)
{
	ActionQueue.run(PostItemScript(script, ref));
}

unsigned char PostStepScript(objref ref, unsigned char dir, int dz, int count)
{
	unsigned char z = dz;

	if (count == 1) {
		return ActionQueue.add(ref.off, (char *)MakeScript(SCRIPT_STEP, (unsigned char)(dir + 0x30), z, SCRIPT_END));
	} else {
		unsigned char script[128];
		script[0] = 1;
		for (int i = 0; i < count; i++) {
			AppendScriptByte(script, SCRIPT_STEP);
			AppendScriptByte(script, dir + 0x30);
			AppendScriptByte(script, z);
		}
		return ActionQueue.add(ref.off, (char *)script);
	}
}

void StepItem(objref ref, unsigned char dir, int dz, int count)
{
	if (!IsNpcUnconscious(&ref)) {
		if (count > 1) {
			Coord x, y;
			x = Coord(Item_getX(ref) + DirectionDX[dir] * count);
			y = Coord(Item_getY(ref) + DirectionDY[dir] * count);
			int z = GetItemZAndStuff(&ref).z() + dz;
			Item_move(&ref, x, y, z);
			unsigned char script[4];
			script[2] = 1;
			script[3] = dir;
			RunItemScript(script, ref);
		} else {
			unsigned char event = PostStepScript(ref, dir, dz, count);
			ActionQueue.run(event);
		}
	}
}
