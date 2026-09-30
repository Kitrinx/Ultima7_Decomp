/* Black Gate U7.EXE, resident segment 79 (file offsets 0x02bc99 to 0x02bcef, 86 bytes).
 * Borland C++ 2.0 -mm -O -G rebuilds it byte for byte.
 * Folder inferred from Serpent Isle's link groups.
 */

#include "random.h"

#define MK_FP(seg, ofs) ((void _seg *) (seg) + (void near *) (ofs))
#define ITEM(off) ((struct ItemRecord far *) MK_FP(ItemBufferSegment, (off)))
#define EGG(off) ((struct EggRecord far *) MK_FP(ItemBufferSegment, (off)))

#define EGG_HATCHED 0x02

struct ItemRecord {
	int next;
	unsigned char x, y;
	unsigned typeFrame;
	int extra;
};

/* the extra record an egg's item points to */
struct EggRecord {
	unsigned char unusedField1;
	unsigned char flags;
	unsigned char percent;    /* chance of hatching */
};

extern unsigned ItemBufferSegment;

int Egg_rollChance(struct EggRecord far *egg)
{
	return GenerateRandomIntegerInRange(100) < egg->percent;
}

void Egg_setHatched(int *ref)
{
	EGG(ITEM(*ref)->extra)->flags |= EGG_HATCHED;
}

void Egg_clearHatched(int *ref)
{
	EGG(ITEM(*ref)->extra)->flags &= ~EGG_HATCHED;
}
