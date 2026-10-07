/* Serpent Isle SI.EXE, resident segment 6 (file offsets 0x00cc02 to 0x00e33c, 5946 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "u7port.h"
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
#include "u7ibuf.h"
#include "itable.h"

typedef uint8_t boolean;

#define SHAPE_AMULET        955
#define SHAPE_F_AUTOMATON   658
#define SHAPE_M_AUTOMATON   747
#define SHAPE_MANSPIDER     979

/* where a type class 11 item stands, kept in its extra record */
struct PositionRecord { uint8_t cellX, cellY, region; };

extern int8_t Item_getHitPoints(objref *ref);
extern void Item_setHitPoints(objref *, uint8_t);
extern uint8_t Item_getQualityFlags(objref *);

extern Coord Item_getX(objref &);
extern Coord Item_getY(objref &);
extern uint8_t Item_move(objref *, Loc, Loc, int16_t);
extern int16_t GetClothingWarmth(objref npc);

/* Virtue stones and amulets remember where they were marked. */
inline boolean KeepsPosition(objref *ref)
{
	return (uint8_t)(gItemTypeInfo[ref->ptr()->typeFrame & 0x3ff].typeClass == TYPE_CLASS_VIRTUE_STONE)
		|| (ref->ptr()->typeFrame & 0x3ff) == SHAPE_AMULET;
}

inline boolean HasNoHitPoints(objref *ref) { return Item_getHitPoints(ref) <= 0; }
inline uint8_t GetItemFlags(objref *ref, uint8_t mask)
{
	return Item_getQualityFlags(ref) & mask;
}

uint8_t objref::cellX()
{
	uint8_t value = 0;

	if (KeepsPosition(this)) {
		int16_t position = ptr()->data.extra;
		value = ((PositionRecord *)ItemAt(position))->cellX;
	}
	return value;
}

void objref::cellX(int16_t value)
{
	if (KeepsPosition(this)) {
		int16_t position = ptr()->data.extra;
		((PositionRecord *)ItemAt(position))->cellX = value;
	}
}

uint8_t objref::cellY()
{
	uint8_t value = 0;

	if (KeepsPosition(this)) {
		int16_t position = ptr()->data.extra;
		value = ((PositionRecord *)ItemAt(position))->cellY;
	}
	return value;
}

void objref::cellY(int16_t value)
{
	if (KeepsPosition(this)) {
		int16_t position = ptr()->data.extra;
		((PositionRecord *)ItemAt(position))->cellY = value;
	}
}

uint8_t objref::region()
{
	uint8_t value = 0;

	if (KeepsPosition(this)) {
		int16_t position = ptr()->data.extra;
		value = ((PositionRecord *)ItemAt(position))->region;
	}
	return value;
}

void objref::region(uint8_t value)
{
	if (KeepsPosition(this)) {
		int16_t position = ptr()->data.extra;
		((PositionRecord *)ItemAt(position))->region = value;
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
	uint8_t area = GetRegionAt(x, y, CurrentMap);

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

void TextPrinter::printChar(int8_t c)
{
	drawChar(target, x, y, c);
	if (c == '\n') {
		y += charHeight(c) + leading;
		x = target->clip.x;
	} else if (c == '\r') {
		x = target->clip.x;
	} else {
		x += charWidth(c) + spacing;
	}
}

void TextPrinter::printString(char *text)
{
	for (; *text; text++)
		printChar(*text);
}

int16_t TextPrinter::textWidth(char *text)
{
	int16_t width = 0;

	for (; *text; text++) {
		width += charWidth(*text);
		width += spacing;
	}
	return width;
}

void UpdateNPCStatus()
{
	NPCRef ref;
	int8_t health, strength, healed, increment;
	boolean recover, wasDead;
	uint8_t mana, maxMana;
	uint16_t type;

	recover = GameTime.ticks % TICKS_PER_HOUR == 0;
	for (int16_t number = 0; number < NPC_COUNT; number++) {
		GetNpcIbo(&ref, number);
		if (ref.valid() && !AvatarDontMove) {
			if (recover) {
				health = Item_getHitPoints(&ref);
				if (health <= 0 || !HasStatus(&ref, NPC_IN_PARTY) || IsWellFed(&ref)) {
					strength = GetNpcBufferForIbo(&ref)->strength & 0x1f;
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
								NpcBuffer *record = GetNpcBufferForIbo(&ref);
								record->changeFlags(NPC_ASLEEP, 0);
							}
						}
					}
				}
				if (ref.isAvatar()) {
					mana = GetNpcBufferForIbo(&ref)->mana & 0x1f;
					maxMana = GetNpcBufferForIbo(&ref)->magic & 0x1f;
					if (mana < maxMana) {
						mana = (uint8_t)(mana + maxMana + 1) / 2;
						if ((uint8_t)(GetNpcBufferForIbo(&ref)->mana & 0x1f) == mana) {
							++mana;
						}
						GetNpcBufferForIbo(&ref)->mana = mana | (GetNpcBufferForIbo(&ref)->mana & 0xe0);
					}
				}
				if (HasStatus(&ref, NPC_IN_PARTY) && GetNpcBufferForIbo(&ref)->food > 0 &&
					ref.type() != SHAPE_F_AUTOMATON && ref.type() != SHAPE_M_AUTOMATON) {
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
				if (HasStatus(&ref, NPC_POISONED) && (GenerateRandomIntegerInRange(10000) < 5 ||
					ref.type() == SHAPE_F_AUTOMATON || ref.type() == SHAPE_M_AUTOMATON)) {
					EndPoison(ref);
				}
				if (HasStatus(&ref, NPC_POISONED) && GenerateRandomIntegerInRange(1000) < 5 &&
					(HasStatus(&ref, NPC_IN_PARTY) ||
					!HasStatus(&ref, NPC_IN_PARTY) && Item_getHitPoints(&ref) > 1)) {
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
					!(uint8_t)HasNPCStepped(Item_getNpcNumber(&ref))) {
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
					if (GetNpcBufferForIbo(&ref)->workType != WORK_WAIT) {
						Npc_setSchedule(&ref, type == SHAPE_MANSPIDER || RollChance(4) ? 0 : 12);
						GetNpcBufferForIbo(&ref)->schedules[GetNpcBufferForIbo(&ref)->currentSchedule].state = 1;
					}
				}
				if (HasStatus(&ref, NPC_IN_PARTY)) {
					NPCRef avatar;
					GetNpcIbo(&avatar, 0);
					if ((ref.type() != SHAPE_F_AUTOMATON ||
						ref.type() == SHAPE_F_AUTOMATON && Npc_hasPetraFlag(&AvatarRef)) &&
						ref.type() != SHAPE_M_AUTOMATON &&
						(!ref.isAvatar() || ref.isAvatar() && !Npc_hasPetraFlag(&ref))) {
						/* The packed mana counts how cold the NPC is; the barks are text 1/158-197. */
						int16_t bark = -1;
						if (Npc_hasFreezeFlag(&avatar)) {
							if (GenerateRandomIntegerInRange(1000) < 100 - GetClothingWarmth(ref)) {
								Npc_incrementTemperature(&ref);
								if (Npc_getTemperature(&ref) == 4)
									bark = Npc_hasZombieFlag(&ref) ? 182 : GenerateRandomIntegerInRange(3) + 158;
								if (GenerateRandomIntegerInRange(5) == 0) {
									if (Npc_packedManaLowRange(&ref))
										bark = Npc_hasZombieFlag(&ref) ? GenerateRandomIntegerInRange(2) + 182
											: GenerateRandomIntegerInRange(3) + 158;
									if (Npc_packedManaMiddleRange(&ref))
										bark = Npc_hasZombieFlag(&ref) ? GenerateRandomIntegerInRange(2) + 184
											: GenerateRandomIntegerInRange(3) + 161;
									if (Npc_packedManaUpperRange(&ref))
										bark = Npc_hasZombieFlag(&ref) ? GenerateRandomIntegerInRange(2) + 186
											: GenerateRandomIntegerInRange(3) + 164;
								}
								if (Npc_packedManaHighRange(&ref)) {
									bark = Npc_hasZombieFlag(&ref) ? GenerateRandomIntegerInRange(2) + 188
										: GenerateRandomIntegerInRange(3) + 167;
									ReduceHealth(ref, 1, 3, 0);
								}
							}
							if (GetClothingWarmth(ref) >= 100) {
								if (Npc_getTemperature(&ref) > 49)
									Npc_setTemperature(&ref, 49);
								if (GetClothingWarmth(ref) > 115 &&
									GenerateRandomIntegerInRange(1000) < GetClothingWarmth(ref) / 2 + 25) {
									Npc_decrementTemperature(&ref);
									if (Npc_getTemperature(&ref) == 49)
										bark = Npc_hasZombieFlag(&ref) ? GenerateRandomIntegerInRange(2) + 196 : 181;
									if (Npc_getTemperature(&ref) == 39)
										bark = Npc_hasZombieFlag(&ref) ? GenerateRandomIntegerInRange(2) + 194 : 177;
									if (Npc_getTemperature(&ref) == 24)
										bark = Npc_hasZombieFlag(&ref) ? GenerateRandomIntegerInRange(2) + 192 : 175;
									if (Npc_getTemperature(&ref) == 14)
										bark = Npc_hasZombieFlag(&ref) ? GenerateRandomIntegerInRange(2) + 190 : 170;
								}
							}
						} else if (Npc_getTemperature(&ref) > 0 &&
							GenerateRandomIntegerInRange(1000) < GetClothingWarmth(ref) + 125) {
							Npc_decrementTemperature(&ref);
							if (Npc_getTemperature(&ref) > 49)
								Npc_setTemperature(&ref, 49);
							if (Npc_getTemperature(&ref) == 49)
								bark = Npc_hasZombieFlag(&ref) ? GenerateRandomIntegerInRange(2) + 196
									: GenerateRandomIntegerInRange(3) + 179;
							if (Npc_getTemperature(&ref) == 39)
								bark = Npc_hasZombieFlag(&ref) ? GenerateRandomIntegerInRange(2) + 194
									: GenerateRandomIntegerInRange(3) + 176;
							if (Npc_getTemperature(&ref) == 24)
								bark = Npc_hasZombieFlag(&ref) ? GenerateRandomIntegerInRange(2) + 192
									: GenerateRandomIntegerInRange(3) + 173;
							if (Npc_getTemperature(&ref) == 14)
								bark = Npc_hasZombieFlag(&ref) ? GenerateRandomIntegerInRange(2) + 190
									: GenerateRandomIntegerInRange(3) + 170;
						}
						if (bark != -1)
							SpriteManager_barkOnItem(&gSpriteManager, ref.off, GetGameText(1, bark), 0, 15, 0);
					}
					if (GetNpcBufferForIbo(&ref)->workType == WORK_COMBAT) {
						if (IsFamished(&ref) && Item_getHitPoints(&ref) > 0
							&& GenerateRandomIntegerInRange(1000) < 15) {
							ReduceHealth(ref, 1, 3, 0);
						}
					} else if (ref.type() != SHAPE_F_AUTOMATON && ref.type() != SHAPE_M_AUTOMATON) {
						if (IsHungry(&ref)) {
							if (GenerateRandomIntegerInRange(1000) < 4) {
								SpriteManager_barkOnItem(&gSpriteManager, ref.off,
									GetGameText(1, GenerateRandomIntegerInRange(3) + 119), 0, 15, 0);
							}
						} else if (IsStarving(&ref)) {
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
}

void HidePointer()
{
	HideCursor();
}

void ShowPointer()
{
	ShowCursor();
}

void SetPointerZ(int16_t value)
{
	AdjustCursorZ(value - CursorCenterZ);
}

void ShiftPointerZ(int8_t value)
{
	AdjustCursorZ(value);
}

uint16_t GetCursorLength()
{
	return ArrowLength;
}

void HideMarkerShapes()
{
	RemapShapeRecord(275, 239);
	RemapShapeRecord(200, 239);
	RemapShapeRecord(961, 239);
	RemapShapeRecord(607, 239);
}

uint8_t PostItemScript(uint8_t *script, objref ref)
{
	script[0] = 4;
	script[1] = 0x57;
	return ActionQueue.add(ref.off, (char *)script);
}

extern "C" void RunItemScript(uint8_t *script, objref ref)
{
	ActionQueue.run(PostItemScript(script, ref));
}

uint8_t PostStepScript(objref ref, uint8_t dir, int16_t dz, int16_t count)
{
	uint8_t z = dz;

	if (count == 1) {
		return ActionQueue.add(ref.off, (char *)MakeScript(SCRIPT_STEP, (uint8_t)(dir + 0x30), z, SCRIPT_END));
	} else {
		uint8_t script[128];
		script[0] = 1;
		for (int16_t i = 0; i < count; i++) {
			AppendScriptByte(script, SCRIPT_STEP);
			AppendScriptByte(script, dir + 0x30);
			AppendScriptByte(script, z);
		}
		return ActionQueue.add(ref.off, (char *)script);
	}
}

void StepItem(objref ref, uint8_t dir, int16_t dz, int16_t count)
{
	if (!IsNpcUnconscious(&ref)) {
		if (count > 1) {
			Coord x, y;
			x = Coord(Item_getX(ref) + DirectionDX[dir] * count);
			y = Coord(Item_getY(ref) + DirectionDY[dir] * count);
			int16_t z = GetItemZAndStuff(&ref).z() + dz;
			Item_move(&ref, x, y, z);
			uint8_t script[4];
			script[2] = 1;
			script[3] = dir;
			RunItemScript(script, ref);
		} else {
			uint8_t event = PostStepScript(ref, dir, dz, count);
			ActionQueue.run(event);
		}
	}
}
