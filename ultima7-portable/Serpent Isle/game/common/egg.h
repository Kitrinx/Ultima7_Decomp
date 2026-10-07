#ifndef EGG_H
#define EGG_H

union EggData {
	uint16_t word;
	struct { uint8_t first, second; } bytes;
};

/* Packed trigger metadata, chance and two payload words. */
struct EggRecord {
	uint16_t kind : 4, criteria : 3, nocturnal : 1;
	uint16_t once : 1, hatched : 1, distance : 5, repeat : 1;
	uint8_t probability;
	union EggData data1;
	uint8_t unusedByte;
	union EggData data2;
};

#ifdef __cplusplus
extern "C" {
#endif
int16_t Egg_rollChance(struct EggRecord *egg);
void Egg_setHatched(int16_t *ref);
void Egg_clearHatched(int16_t *ref);
void Egg_setCriteriaAndDistance(int16_t *ref, uint8_t criteria, uint8_t distance);
#ifdef __cplusplus
}
#endif

#endif
