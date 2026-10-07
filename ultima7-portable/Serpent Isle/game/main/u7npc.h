#ifndef U7NPC_H
#define U7NPC_H

#include "activity.h"
#include "typefram.h"
#include "coord.h"

struct NpcSaver;

#define NPC_COUNT           356

/* NPC status bits; bits 0-2 hold the facing and bits 3-4 the alignment. */
#define NPC_ASLEEP      0x80
#define NPC_CHARMED     0x100
#define NPC_CURSED      0x200
#define NPC_IN_PARTY    0x800
#define NPC_PARALYZED   0x1000
#define NPC_POISONED    0x2000
#define NPC_PROTECTED   0x4000
#define NPC_DEAD        0x8000

/* NPC type flags. */
#define NPC_FLY         0x10
#define NPC_WALK        0x20
#define NPC_SWIM        0x40
#define NPC_ETHEREAL    0x80

struct objref;
struct NPCRef;
struct Loc;
struct CellCoord;
struct TypeFrame;

/* One slot of an NPC's schedule stack; bytes 2-6 belong to the running schedule. */
struct ScheduleSlot {
	uint8_t kind, state;
	int16_t x, y;
	uint8_t counter;
};

/* The 105-byte NPC buffer record: NPC.DAT record bytes 12 onward. */
struct NpcBuffer {
	int16_t nextFree;
	int16_t ref;
	uint16_t status;
	uint8_t strength, dexterity, intelligence, combat;
	uint8_t workType, defaultAttackMode;
	uint16_t type;
	uint8_t hitPoints;
	uint8_t magic, mana, face;
	uint8_t scheduleC;
	int8_t unusedByte;
	int32_t experience;
	uint8_t training;
	int16_t primaryTarget, secondaryTarget, oppressor;
	int16_t iVr[2], sVr[2];
	uint8_t typeFlags, typeFlagsHigh;
	int8_t direction;
	uint16_t schedulePosition;
	int16_t scheduleFlags;
	ScheduleSlot schedules[5];
	uint8_t food;
	uint8_t currentSchedule;
	int16_t result;
	uint16_t scheduleValue, scheduleToggle;
	char name[16];
	void changeFlags(uint16_t clear, uint16_t set) {
		status &= ~clear;
		status |= set;
	}
	/* Alignment lives in status bits 3-4. */
	void setAlignment(uint8_t n) {
		status &= ~0x18;
		status |= (n << 3) & 0x18;
	}
	void setInitialAlignment(uint8_t n) {
		status &= ~0x60;
		status |= (n << 5) & 0x60;
	}
	void setType(TypeFrame value) { type = value.bits; }
	/* Status bits 0-2 hold the facing. */
	uint8_t facing() { return status & 7; }
	uint8_t hasStatus(uint16_t mask) { return (status & mask) != 0; }
	uint8_t moving(uint8_t mask) { return typeFlags & mask; }
	uint8_t marked(uint8_t mask) { return typeFlagsHigh & mask; }
};

/* Where the NPC buffers sit in the memory manager. */
struct NpcBufferPool {
	int32_t address;
	NpcBufferPool() { address = 0; }
};

extern int16_t FreeNpcNumbers;
extern int16_t FreeMonsterNumbers;
extern NpcBufferPool NpcPool;

void Npc_setSchedule(objref *who, int8_t activity);
void Npc_pushSchedule(objref *who, int8_t activity, int16_t x, int16_t y, int8_t counter);
void Npc_popSchedule(objref *who, int16_t result);

NpcBuffer * GetNpcBufferForIbo(objref *ref);
uint8_t Npc_getStrength(objref *ref);
uint8_t Npc_getDexterity(objref *ref);
uint8_t Npc_getIntelligence(objref *ref);
uint8_t Npc_getCombat(objref *ref);
int16_t Npc_getLevel(objref *ref);
int16_t Item_isAvatar(objref *ref);
uint8_t CreateNpc(objref *ref, TypeFrame &shape, CellCoord x, CellCoord y, int16_t z);
inline uint8_t CreateNpc(objref *ref, TypeFrame &&shape, CellCoord x, CellCoord y, int16_t z)
{
	return CreateNpc(ref, shape, x, y, z);
}
void GetNpcIbo(objref *ref, int16_t number);
int16_t Item_getNpcNumber(objref *ref);
uint8_t Npc_hasItemTarget(objref *ref);
int16_t Npc_getItemTarget(objref *ref);
void Npc_getTargetCoords(objref *ref, int16_t *x, int16_t *y, int16_t *z);
void Npc_setItemTarget(objref *ref, int16_t target);
void Npc_setCoordTarget(objref *ref, int16_t *x, int16_t *y, int16_t z);
void Npc_setTargetWeapon(objref *ref, uint16_t value);
void Npc_setMale(objref *ref);
void Npc_setFemale(objref *ref);
uint8_t Npc_isMale(objref *ref);
uint8_t IsNpcUnconscious(objref *ref);

uint8_t FindAvatarNpc(void);
void Npc_clearTargets(objref *who);

void Npc_runSchedule(objref *who);

/* The NPC's last path failed, or type flag 8 is set. */
inline int8_t IsBlocked(objref *who)
{
	return GetNpcBufferForIbo(who)->result == -1 || GetNpcBufferForIbo(who)->marked(8);
}

/* Accessors for an NPC's buffer record. */
inline uint8_t IsInParty(objref *who) { return (GetNpcBufferForIbo(who)->status & NPC_IN_PARTY) != 0; }
inline uint8_t IsDead(objref *who) { return (GetNpcBufferForIbo(who)->status & NPC_DEAD) != 0; }
inline uint8_t GetAlignment(objref *who) { return (GetNpcBufferForIbo(who)->status & 0x18) >> 3; }
inline uint8_t GetFacing(objref *who) { return GetNpcBufferForIbo(who)->status & 7; }
inline uint8_t HasTarget(objref *who) { return GetNpcBufferForIbo(who)->primaryTarget != -1; }
inline uint8_t HasSecondary(objref *who) { return GetNpcBufferForIbo(who)->secondaryTarget != -1; }
inline uint8_t WantsPrimary(objref *who) { return GetNpcBufferForIbo(who)->typeFlagsHigh & 1; }
inline uint8_t HasStatus(objref *who, uint16_t mask) { return (GetNpcBufferForIbo(who)->status & mask) != 0; }
inline void ChangeStatus(objref *who, uint16_t clear, uint16_t set)
{
	NpcBuffer *npc = GetNpcBufferForIbo(who);
	npc->status &= ~clear;
	npc->status |= set;
}
inline uint8_t IsOnOutermostSchedule(objref *who) { return GetNpcBufferForIbo(who)->currentSchedule == 0; }
inline uint8_t IsInCombat(objref *who)
{
	return GetNpcBufferForIbo(who)->workType == WORK_COMBAT && IsOnOutermostSchedule(who);
}

/* Food 10 and up is fed, 5 to 9 hungry, 1 to 4 starving and 0 famished. */
inline uint8_t IsWellFed(objref *who) { return GetNpcBufferForIbo(who)->food >= 10; }
inline uint8_t IsHungry(objref *who)
{
	return GetNpcBufferForIbo(who)->food < 10 && GetNpcBufferForIbo(who)->food >= 5;
}
inline uint8_t IsStarving(objref *who)
{
	return GetNpcBufferForIbo(who)->food < 5 && GetNpcBufferForIbo(who)->food > 0;
}
inline uint8_t IsFamished(objref *who) { return GetNpcBufferForIbo(who)->food == 0; }

void DetachItem(int16_t item);
void SendNPCToLunch(NPCRef npc);

extern char *const U7NBufFileName;
extern NpcSaver NpcBufferFile;

int16_t Npc_getScheduleVariable0(objref *ref);
int16_t Npc_getScheduleVariable1(objref *ref);

#endif
