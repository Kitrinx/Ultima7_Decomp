#ifndef NPCREF_H
#define NPCREF_H

#include "objref.h"
#include "u7npc.h"

/* An item known to be an NPC. */
struct NPCRef : objref {
	NPCRef() {}
	NPCRef(objref r) { off = r.off; }

	NpcBuffer far *buffer();
	void changeFlags(unsigned clear, unsigned set);
	unsigned char flag(unsigned mask);
	int number();
	unsigned char isAvatar();
	unsigned char strength();
	unsigned char dexterity();
	unsigned char intelligence();
	unsigned char combat();
	unsigned char unconscious();
};

void far Item_moveOffMap(objref *ref);
void far GetNpcIbo(objref *ref, int number);
unsigned char far Npc_isMale(objref *ref);

void far Npc_freeNumber(objref *ref);
void far Npc_freeMonsterNumber(objref *ref);
int far Item_isAvatar(objref *ref);
int far Item_getNpcNumber(objref *ref);
unsigned char far IsNpcUnconscious(objref *ref);
void far Npc_changeFood(objref *ref, char amount);

inline NpcBuffer far *NPCRef::buffer() { return GetNpcBufferForIbo(this); }
inline void NPCRef::changeFlags(unsigned clear, unsigned set) { ChangeStatus(this, clear, set); }
inline unsigned char NPCRef::flag(unsigned mask) { return HasStatus(this, mask); }
inline int NPCRef::number() { return Item_getNpcNumber(this); }
inline unsigned char NPCRef::isAvatar() { return Item_isAvatar(this); }
inline unsigned char NPCRef::strength() { return Npc_getStrength(this); }
inline unsigned char NPCRef::dexterity() { return Npc_getDexterity(this); }
inline unsigned char NPCRef::intelligence() { return Npc_getIntelligence(this); }
inline unsigned char NPCRef::combat() { return Npc_getCombat(this); }
inline unsigned char NPCRef::unconscious() { return IsNpcUnconscious(this); }

struct NpcView;

void far StoreNpcViewRecord(long *address, int index, NpcView *record);

struct Coord;

void far Npc_setCoordTarget(objref *ref, Coord x, Coord y, int z);

int far Npc_getTargetWeapon(objref *ref);

extern int FacingFrameOffsets[];
extern objref AvatarRef;
extern int FreeNpcNumbers;
extern int FreeMonsterNumbers;
extern NpcBufferPool NpcPool;
extern NpcBuffer NullNpcBuffer;
NpcBuffer far *far GetNpcBufferForIbo(objref *ref);
void far Npc_allocateNumber(objref *ref);
void far Npc_allocateMonsterNumber(objref *ref);
unsigned char far Npc_getStrength(objref *ref);
unsigned char far Npc_getDexterity(objref *ref);
unsigned char far Npc_getIntelligence(objref *ref);
unsigned char far Npc_getCombat(objref *ref);
int far Npc_getLevel(objref *ref);
unsigned char far CreateNpc(objref *ref, TypeFrame far &shape, CellCoord x, CellCoord y, int z);
void far Npc_setNumber(objref *ref, unsigned number);
unsigned char far Npc_hasItemTarget(objref *ref);
int far Npc_getItemTarget(objref *ref);
void far Npc_getTargetCoords(objref *ref, int *x, int *y, int *z);
void far Npc_setItemTarget(objref *ref, int target);
void far Npc_setTargetWeapon(objref *ref, unsigned value);
void far Npc_setMale(objref *ref);
void far Npc_setFemale(objref *ref);

unsigned char far Npc_hasMetFlag(objref *ref);
void far Npc_setMetFlag(objref *ref);
void far Npc_clearMetFlag(objref *ref);
unsigned char far Npc_hasNoCastFlag(objref *ref);
void far Npc_setNoCastFlag(objref *ref);
void far Npc_clearNoCastFlag(objref *ref);
unsigned char far Npc_getSkinColor(objref *ref);
void far Npc_setSkinColor(objref *ref, unsigned char value);
unsigned char far Npc_hasZombieFlag(objref *ref);
void far Npc_setZombieFlag(objref *ref);
void far Npc_clearZombieFlag(objref *ref);
unsigned char far Npc_hasFreezeFlag(objref *ref);
void far Npc_setFreezeFlag(objref *ref);
void far Npc_clearFreezeFlag(objref *ref);
unsigned char far Npc_hasReadFlag(objref *ref);
void far Npc_setReadFlag(objref *ref);
void far Npc_clearReadFlag(objref *ref);
unsigned char far Npc_hasPetraFlag(objref *ref);
void far Npc_setPetraFlag(objref *ref);
void far Npc_clearPetraFlag(objref *ref);
unsigned char far Npc_hasCombatLowFlag(objref *ref);
void far Npc_setCombatLowFlag(objref *ref);
void far Npc_clearCombatLowFlag(objref *ref);
unsigned char far Npc_hasPolymorphFlag(objref *ref);
void far Npc_setPolymorphFlag(objref *ref);
void far Npc_clearPolymorphFlag(objref *ref);
unsigned char far Npc_hasTournamentFlag(objref *ref);
void far Npc_setTournamentFlag(objref *ref);
void far Npc_clearTournamentFlag(objref *ref);
unsigned char far Npc_getTemperature(objref *ref);
void far Npc_setTemperature(objref *ref, unsigned char value);
void far Npc_incrementTemperature(objref *ref);
void far Npc_decrementTemperature(objref *ref);
unsigned char far Npc_packedManaLowRange(objref *ref);
unsigned char far Npc_packedManaMiddleRange(objref *ref);
unsigned char far Npc_packedManaUpperRange(objref *ref);
unsigned char far Npc_packedManaHighRange(objref *ref);
unsigned char far Npc_getMagic(objref *ref);
void far Npc_setMagic(objref *ref, unsigned char value);

#endif
