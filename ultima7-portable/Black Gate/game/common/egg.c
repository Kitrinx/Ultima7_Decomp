/* Black Gate U7.EXE, resident segment 79 (file offsets 0x02bc99 to 0x02bcef, 86 bytes).
 * Borland C++ 2.0 -mm -O -G rebuilds it byte for byte.
 * Folder inferred from Serpent Isle's link groups.
 */

#include "u7port.h"
#include "random.h"
#include "itembase.h"

#define ITEM(off) ((struct ItemRecord *) ItemAt((off)))
#define EGG(off) ((struct EggRecord *) ItemAt((off)))

#define EGG_HATCHED 0x02

struct ItemRecord {
	int16_t next;
	uint8_t x, y;
	uint16_t typeFrame;
	int16_t extra;
};

/* the extra record an egg's item points to */
struct EggRecord {
	uint8_t unusedField1;
	uint8_t flags;
	uint8_t percent;    /* chance of hatching */
};

extern uint8_t *ItemBufferBase;

int16_t Egg_rollChance(struct EggRecord *egg)
{
	return GenerateRandomIntegerInRange(100) < egg->percent;
}

void Egg_setHatched(int16_t *ref)
{
	EGG(ITEM(*ref)->extra)->flags |= EGG_HATCHED;
}

void Egg_clearHatched(int16_t *ref)
{
	EGG(ITEM(*ref)->extra)->flags &= ~EGG_HATCHED;
}
