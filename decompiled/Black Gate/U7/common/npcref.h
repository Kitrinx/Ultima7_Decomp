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

#endif
