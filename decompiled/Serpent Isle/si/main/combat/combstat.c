/* Serpent Isle SI.EXE, overlay segment 353 (file offsets 0x0ad080 to 0x0ad2c6, 582 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "activity.h"
#include "iteminfo.h"
#include "u7npc.h"
#include "combat.h"
#include "item.h"
#include "combmode.h"
#include "npcref.h"
#include "mapview.h"

#define IS_VALID(r) ((char) ((r) != 0))
#define KIND(b) ((char) ((int) (b) & 7))
#define IS_KIND(b, k) ((char) (KIND(b) == (k)))
#define FLAG(v, m) ((unsigned char) ((v) & (m)))
#define NPC(r) GetNpcBufferForIbo((objref *)(r))
#define ALIGNMENT(r) ((unsigned char) ((NPC(r)->status & 0x18) >> 3))
#define IS_ATTACK_MODE(r, m) ((unsigned char) (Item_getQuality((objref *)(r)) == (m)))

inline char GetAttackMode(NPCRef r)
{
	return Item_getQuality((objref *)&r.off);
}

unsigned char IsAvatarInCombat(void)
{
	return NPC(&AvatarRef)->workType == WORK_COMBAT;
}

void Combat::unmarkNPC(unsigned n)
{
	unsigned word, bit;

	word = n >> 4;
	bit = n - (word << 4);
	marks[word] = marks[word] & ~(1 << bit);
}

void Combat::markNPC(unsigned n)
{
	unsigned word, bit;

	word = n >> 4;
	bit = n - (word << 4);
	marks[word] = marks[word] | (1 << bit);
}

char Combat::isNPCMarked(unsigned n)
{
	int word, bit;

	word = n >> 4;
	bit = n - word * 16;
	if (marks[word] & (1 << bit))
		return 1;
	return 0;
}

void Combat::clearMovePoints()
{
	int i;

	for (i = 0; i < NPC_COUNT; i++)
		movePoints[i] = 0;
}

unsigned char IsAvatarAutoAttack(void)
{
	return GetAttackMode(AvatarRef) != ATTACK_MANUAL;
}

/* evil and chaotic NPCs still in the fight */
int Combat::countEnemies()
{
	int npc;
	ItemInfo info;
	int i, count;

	count = 0;
	for (i = 0; i < NPC_COUNT; i++) {
		GetNpcIbo((objref *)&npc, i);
		if (IS_VALID(npc)) {
			info = GetItemZAndStuff((objref *)&npc);
			if (!(IS_KIND(info.flags, 4) ? 0 : (int) IsItemInCellWindow(npc)))
				continue;
			if (NPC(&npc)->workType == WORK_COMBAT && (ALIGNMENT(&npc) == 2 || ALIGNMENT(&npc) == 3)
				&& !IsNpcUnconscious((objref *)&npc) && !IS_ATTACK_MODE(&npc, ATTACK_FLEE))
				count++;
		}
	}
	return count;
}
