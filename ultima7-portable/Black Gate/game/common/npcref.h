#ifndef NPCREF_H
#define NPCREF_H

#include "objref.h"
#include "u7npc.h"

/* An item known to be an NPC. */
struct NPCRef : objref {
	NPCRef() {}
	NPCRef(objref r) { off = r.off; }

	NpcBuffer *buffer();
	void changeFlags(uint16_t clear, uint16_t set);
	uint8_t flag(uint16_t mask);
	int16_t number();
	uint8_t isAvatar();
	uint8_t strength();
	uint8_t dexterity();
	uint8_t intelligence();
	uint8_t combat();
	uint8_t unconscious();
};

void Item_moveOffMap(objref *ref);
void GetNpcIbo(objref *ref, int16_t number);
uint8_t Npc_isMale(objref *ref);

void Npc_freeNumber(objref *ref);
void Npc_freeMonsterNumber(objref *ref);
int16_t Item_isAvatar(objref *ref);
int16_t Item_getNpcNumber(objref *ref);
uint8_t IsNpcUnconscious(objref *ref);
void Npc_changeFood(objref *ref, int8_t amount);

inline NpcBuffer *NPCRef::buffer() { return GetNpcBufferForIbo(this); }
inline void NPCRef::changeFlags(uint16_t clear, uint16_t set) { ChangeStatus(this, clear, set); }
inline uint8_t NPCRef::flag(uint16_t mask) { return HasStatus(this, mask); }
inline int16_t NPCRef::number() { return Item_getNpcNumber(this); }
inline uint8_t NPCRef::isAvatar() { return Item_isAvatar(this); }
inline uint8_t NPCRef::strength() { return Npc_getStrength(this); }
inline uint8_t NPCRef::dexterity() { return Npc_getDexterity(this); }
inline uint8_t NPCRef::intelligence() { return Npc_getIntelligence(this); }
inline uint8_t NPCRef::combat() { return Npc_getCombat(this); }
inline uint8_t NPCRef::unconscious() { return IsNpcUnconscious(this); }

struct NpcView;

void StoreNpcViewRecord(int32_t *address, int16_t index, NpcView *record);

struct Coord;

void Npc_setCoordTarget(objref *ref, Coord x, Coord y, int16_t z);

int16_t Npc_getTargetWeapon(objref *ref);

extern const int16_t FacingFrameOffsets[];
extern objref AvatarRef;
extern int16_t FreeNpcNumbers;
extern int16_t FreeMonsterNumbers;
extern NpcBufferPool NpcPool;
extern NpcBuffer NullNpcBuffer;
NpcBuffer * GetNpcBufferForIbo(objref *ref);
void Npc_allocateNumber(objref *ref);
void Npc_allocateMonsterNumber(objref *ref);
uint8_t Npc_getStrength(objref *ref);
uint8_t Npc_getDexterity(objref *ref);
uint8_t Npc_getIntelligence(objref *ref);
uint8_t Npc_getCombat(objref *ref);
int16_t Npc_getLevel(objref *ref);
uint8_t CreateNpc(objref *ref, TypeFrame &shape, CellCoord x, CellCoord y, int16_t z);
void Npc_setNumber(objref *ref, uint16_t number);
uint8_t Npc_hasItemTarget(objref *ref);
int16_t Npc_getItemTarget(objref *ref);
void Npc_getTargetCoords(objref *ref, int16_t *x, int16_t *y, int16_t *z);
void Npc_setItemTarget(objref *ref, int16_t target);
void Npc_setTargetWeapon(objref *ref, uint16_t value);
void Npc_setMale(objref *ref);
void Npc_setFemale(objref *ref);

#endif
