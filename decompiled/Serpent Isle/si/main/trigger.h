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
	unsigned word;
	struct { unsigned char first, second; } bytes;
};

/* An egg's extra record in the item buffer. A monster egg's data1 holds count << 2 | alignment and a
 * work type, its data2 the type and frame; a teleport egg's data1 holds a link (255 none) and a region,
 * its data2 the cell. */
struct EggRecord {
	unsigned kind : 4, criteria : 3, nocturnal : 1;
	unsigned once : 1, hatched : 1, distance : 5, repeat : 1;
	unsigned char probability;
	EggData data1;
	unsigned char unusedByte;
	EggData data2;
};

unsigned char far Egg_hatch(objref ref);
void far Egg_addWaitNear(objref *ref, Script *code);
void far Egg_addWaitAway(objref *ref, Script *code);
#ifdef __cplusplus
extern "C" {
#endif
void far Egg_activate(objref ref, unsigned char force);
void far Egg_setCriteriaAndDistance(int *ref, unsigned char criteria, unsigned char distance);
#ifdef __cplusplus
}
#endif
void far Egg_addMusic(EggRecord far *egg, Script *code);
void far Egg_addSoundEffect(EggRecord far *egg, Script *code);
void far Egg_addSpeech(EggRecord far *egg, Script *code);
void far Egg_addUsecode(EggRecord far *egg, Script *code);
void far Egg_fireMissile(EggRecord far *egg, objref ref, Coord x, Coord y);
void far Egg_addMissileDelay(EggRecord far *egg, Script *code);
void far Egg_addTeleport(EggRecord far *egg, Script *code);
void far Egg_addWeather(EggRecord far *egg, Script *code);
void far Egg_pressButton(EggRecord far *egg, objref ref, Coord x, Coord y);
void far ActivateEgg(int ref);
unsigned char far TriggerEggsUnderItem(objref ref, CellCoord x, CellCoord y);

#endif
