/* Serpent Isle SI.EXE, resident segment 51 (file offsets 0x02180f to 0x0218ae, 159 bytes).
 * Borland C++ 2.0 -mm -O -G -Z rebuilds it byte for byte.
 */

#include "u7port.h"
#include "random.h"
#include "itembase.h"
#include "egg.h"

#define ITEM(off) ((struct ItemRecord *) ItemAt((off)))
#define EGG(off) ((struct EggRecord *) ItemAt((off)))

struct ItemRecord {
	int16_t next;
	uint8_t x, y;
	uint16_t typeFrame;
	int16_t extra;
};


extern uint8_t *ItemBufferBase;

int16_t Egg_rollChance(struct EggRecord *egg)
{
	return GenerateRandomIntegerInRange(100) < egg->probability;
}

void Egg_setHatched(int16_t *ref)
{
	EGG(ITEM(*ref)->extra)->hatched = 1;
}

void Egg_clearHatched(int16_t *ref)
{
	EGG(ITEM(*ref)->extra)->hatched = 0;
}

void Egg_setCriteriaAndDistance(int16_t *ref, uint8_t criteria, uint8_t distance)
{
	if (criteria < 8) {
		EGG(ITEM(*ref)->extra)->criteria = criteria;
		EGG(ITEM(*ref)->extra)->distance = distance;
	}
}
