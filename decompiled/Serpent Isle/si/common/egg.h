#ifndef EGG_H
#define EGG_H

union EggData {
	unsigned word;
	struct { unsigned char first, second; } bytes;
};

/* Packed trigger metadata, chance and two payload words. */
struct EggRecord {
	unsigned kind : 4, criteria : 3, nocturnal : 1;
	unsigned once : 1, hatched : 1, distance : 5, repeat : 1;
	unsigned char probability;
	union EggData data1;
	unsigned char unusedByte;
	union EggData data2;
};

#ifdef __cplusplus
extern "C" {
#endif
int far Egg_rollChance(struct EggRecord far *egg);
void far Egg_setHatched(int *ref);
void far Egg_clearHatched(int *ref);
void far Egg_setCriteriaAndDistance(int *ref, unsigned char criteria, unsigned char distance);
#ifdef __cplusplus
}
#endif

#endif
