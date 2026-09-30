#ifndef TRIGGER_H
#define TRIGGER_H

struct Coord;
struct Script;

struct objref;
struct CellCoord;

/* Egg kinds. */
#define EGG_MONSTER     1
#define EGG_MUSIC       2
#define EGG_SFX         3
#define EGG_SPEECH      4
#define EGG_USECODE     5
#define EGG_MISSILE     6
#define EGG_TELEPORT    7
#define EGG_WEATHER     8
#define EGG_BUTTON      10

/* What hatches an egg: 1 to 3 something coming near, 4 the avatar or 5 any party member stepping on it,
 * 6 an item set on it, 7 a button egg nearby. */
#define CRITERIA_AVATAR_FOOTPAD 4
#define CRITERIA_PARTY_FOOTPAD  5
#define CRITERIA_STEPPED_ON     6
#define CRITERIA_BUTTON         7

/* Two bytes of an egg's data, read as a word or byte by byte. */
union EggData {
	uint16_t word;
	struct { uint8_t first, second; } bytes;
};

/* An egg's extra record in the item buffer. A monster egg's data1 holds count << 2 | alignment and a
 * work type, its data2 the type and frame; a teleport egg's data1 holds a link (255 none) and a region,
 * its data2 the cell. */
struct EggRecord {
	uint16_t kind : 4, criteria : 3, nocturnal : 1;
	uint16_t once : 1, hatched : 1, distance : 5, repeat : 1;
	uint8_t probability;
	EggData data1;
	uint8_t unusedByte;
	EggData data2;
};

uint8_t Egg_hatch(objref ref);
void Egg_addWaitNear(objref *ref, Script *code);
void Egg_addWaitAway(objref *ref, Script *code);
#ifdef __cplusplus
extern "C" {
#endif
void Egg_activate(objref ref, uint8_t force);
#ifdef __cplusplus
}
#endif
void Egg_addMusic(EggRecord *egg, Script *code);
void Egg_addSoundEffect(EggRecord *egg, Script *code);
void Egg_addSpeech(EggRecord *egg, Script *code);
void Egg_addUsecode(EggRecord *egg, Script *code);
void Egg_fireMissile(EggRecord *egg, objref ref, Coord x, Coord y);
void Egg_addMissileDelay(EggRecord *egg, Script *code);
void Egg_addTeleport(EggRecord *egg, Script *code);
void Egg_addWeather(EggRecord *egg, Script *code);
void Egg_pressButton(EggRecord *egg, objref ref, Coord x, Coord y);
void ActivateEgg(int16_t ref);
uint8_t TriggerEggsUnderItem(objref ref, CellCoord x, CellCoord y);

#endif
