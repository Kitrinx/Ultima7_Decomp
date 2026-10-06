/* Serpent Isle SI.EXE, resident segment 51 (file offsets 0x02180f to 0x0218ae, 159 bytes).
 * Borland C++ 2.0 -mm -O -G -Z rebuilds it byte for byte.
 */

#include "random.h"
#include "egg.h"

#define MK_FP(seg, ofs) ((void _seg *) (seg) + (void near *) (ofs))
#define ITEM(off) ((struct ItemRecord far *) MK_FP(ItemBufferSegment, (off)))
#define EGG(off) ((struct EggRecord far *) MK_FP(ItemBufferSegment, (off)))

struct ItemRecord {
	int next;
	unsigned char x, y;
	unsigned typeFrame;
	int extra;
};


extern unsigned ItemBufferSegment;

int Egg_rollChance(struct EggRecord far *egg)
{
	return GenerateRandomIntegerInRange(100) < egg->probability;
}

void Egg_setHatched(int *ref)
{
	EGG(ITEM(*ref)->extra)->hatched = 1;
}

void Egg_clearHatched(int *ref)
{
	EGG(ITEM(*ref)->extra)->hatched = 0;
}

void Egg_setCriteriaAndDistance(int *ref, unsigned char criteria, unsigned char distance)
{
	if (criteria < 8) {
		EGG(ITEM(*ref)->extra)->criteria = criteria;
		EGG(ITEM(*ref)->extra)->distance = distance;
	}
}
