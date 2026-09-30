/* Black Gate U7.EXE, resident segment 13 (file offsets 0x011bd9 to 0x011f87, 942 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder inferred from compiler flags and link order.
 */

#include "u7port.h"
#include "activity.h"
#include "iteminfo.h"
#include "u7npc.h"
#include "combat.h"
#include "item.h"
#include "combmode.h"
#include "npcref.h"
#include "mapview.h"

#define IS_VALID(r) ((int8_t) ((r) != 0))
#define KIND(b) ((int8_t) ((int16_t) (b) & 7))
#define IS_KIND(b, k) ((int8_t) (KIND(b) == (k)))
#define FLAG(v, m) ((uint8_t) ((v) & (m)))
#define NPC(r) GetNpcBufferForIbo((objref *)(r))
#define ALIGNMENT(r) ((uint8_t) ((NPC(r)->status & 0x18) >> 3))
#define IS_ATTACK_MODE(r, m) ((uint8_t) (Item_getQuality((objref *)(r)) == (m)))

inline int8_t GetAttackMode(NPCRef r)
{
	return Item_getQuality((objref *)&r.off);
}

uint8_t IsAvatarInCombat(void)
{
	return NPC(&AvatarRef)->workType == WORK_COMBAT;
}

void Combat::unmarkNPC(uint16_t n)
{
	uint16_t word, bit;

	word = n >> 4;
	bit = n - (word << 4);
	marks[word] = marks[word] & ~(1 << bit);
}

void Combat::markNPC(uint16_t n)
{
	uint16_t word, bit;

	word = n >> 4;
	bit = n - (word << 4);
	marks[word] = marks[word] | (1 << bit);
}

int8_t Combat::isNPCMarked(uint16_t n)
{
	int16_t word, bit;

	word = n >> 4;
	bit = n - word * 16;
	if (marks[word] & (1 << bit))
		return 1;
	return 0;
}

void Combat::clearMarks()
{
	int16_t i;

	for (i = 0; i < 32; i++)
		marks[i] = 0;
}

void Combat::addMovePoints()
{
	int16_t npc;
	uint8_t dexterity;
	ItemInfo info;
	int16_t i;

	for (i = 0; i < NPC_COUNT; i++) {
		GetNpcIbo((objref *)&npc, i);
		if (IS_VALID(npc)) {
			info = GetItemZAndStuff((objref *)&npc);
			if (!(IS_KIND(info.flags, 4) ? 0 : (int16_t) IsItemInCellWindow(npc)))
				continue;
			if (!FLAG(Item_getQualityFlags((objref *)&npc), QUALITY_BUSY) && !IsNpcUnconscious((objref *)&npc)) {
				dexterity = Npc_getDexterity((objref *)&npc);
				if (movePoints[i] + dexterity < 56)
					movePoints[i] += dexterity;
				else
					movePoints[i] = 56;
			}
		}
	}
}

void Combat::clearMovePoints()
{
	int16_t i;

	for (i = 0; i < NPC_COUNT; i++)
		movePoints[i] = 0;
}

int8_t Combat::isProtectee(int16_t n)
{
	int16_t npc;

	GetNpcIbo((objref *)&npc, n);
	if (IS_VALID(npc))
		return leaders[ALIGNMENT(&npc)] == Item_getNpcNumber((objref *)&npc);
	return 0;
}

uint8_t IsAvatarAutoAttack(void)
{
	return GetAttackMode(AvatarRef) != ATTACK_MANUAL;
}

/* evil and chaotic NPCs still in the fight */
int16_t Combat::countEnemies()
{
	int16_t npc;
	ItemInfo info;
	int16_t i, count;

	count = 0;
	for (i = 0; i < NPC_COUNT; i++) {
		GetNpcIbo((objref *)&npc, i);
		if (IS_VALID(npc)) {
			info = GetItemZAndStuff((objref *)&npc);
			if (!(IS_KIND(info.flags, 4) ? 0 : (int16_t) IsItemInCellWindow(npc)))
				continue;
			if (NPC(&npc)->workType == WORK_COMBAT && (ALIGNMENT(&npc) == 2 || ALIGNMENT(&npc) == 3)
				&& !IsNpcUnconscious((objref *)&npc) && !IS_ATTACK_MODE(&npc, ATTACK_FLEE))
				count++;
		}
	}
	return count;
}
