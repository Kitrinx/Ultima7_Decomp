#ifndef U7NPC_H
#define U7NPC_H

#include "activity.h"
#include "typefram.h"

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
struct Loc;
struct CellCoord;
struct far TypeFrame;

/* One slot of an NPC's schedule stack; bytes 2-6 belong to the running schedule. */
struct ScheduleSlot {
	unsigned char kind, state;
	int x, y;
	unsigned char counter;
};

/* The 105-byte NPC buffer record: NPC.DAT record bytes 12 onward. */
struct far NpcBuffer {
	int nextFree;
	int ref;
	unsigned status;
	unsigned char strength, dexterity, intelligence, combat;
	unsigned char workType, defaultAttackMode;
	unsigned type;
	unsigned char hitPoints;
	unsigned char magic, mana, face;
	unsigned char scheduleC;
	char unusedByte;
	long experience;
	unsigned char training;
	int primaryTarget, secondaryTarget, oppressor;
	int iVr[2], sVr[2];
	unsigned char typeFlags, typeFlagsHigh;
	char direction;
	unsigned schedulePosition;
	int scheduleFlags;
	ScheduleSlot schedules[5];
	unsigned char food;
	unsigned char currentSchedule;
	int result;
	unsigned scheduleValue, scheduleToggle;
	char name[16];
	void changeFlags(unsigned clear, unsigned set) {
		status &= ~clear;
		status |= set;
	}
	/* Alignment lives in status bits 3-4. */
	void setAlignment(unsigned char n) {
		status &= ~0x18;
		status |= (n << 3) & 0x18;
	}
	void setInitialAlignment(unsigned char n) {
		status &= ~0x60;
		status |= (n << 5) & 0x60;
	}
	void setType(TypeFrame value) { type = value.bits; }
	/* Status bits 0-2 hold the facing. */
	unsigned char facing() { return status & 7; }
	unsigned char hasStatus(unsigned mask) { return (status & mask) != 0; }
	unsigned char moving(unsigned char mask) { return typeFlags & mask; }
	unsigned char marked(unsigned char mask) { return typeFlagsHigh & mask; }
};

/* Where the NPC buffers sit in the memory manager. */
struct NpcBufferPool {
	long address;
	NpcBufferPool() { address = 0; }
};

extern int FreeNpcNumbers;
extern int FreeMonsterNumbers;
extern NpcBufferPool NpcPool;

void far Npc_setSchedule(objref *who, char activity);
void far Npc_pushSchedule(objref *who, char activity, int x, int y);
void far Npc_popSchedule(objref *who, int result);

NpcBuffer far *far GetNpcBufferForIbo(objref *ref);
unsigned char far Npc_getStrength(objref *ref);
unsigned char far Npc_getDexterity(objref *ref);
unsigned char far Npc_getIntelligence(objref *ref);
unsigned char far Npc_getCombat(objref *ref);
int far Npc_getLevel(objref *ref);
int far Item_isAvatar(objref *ref);
unsigned char far CreateNpc(objref *ref, TypeFrame far &shape, CellCoord x, CellCoord y, int z);
void far GetNpcIbo(objref *ref, int number);
int far Item_getNpcNumber(objref *ref);
unsigned char far Npc_hasItemTarget(objref *ref);
int far Npc_getItemTarget(objref *ref);
void far Npc_getTargetCoords(objref *ref, int *x, int *y, int *z);
void far Npc_setItemTarget(objref *ref, int target);
void far Npc_setCoordTarget(objref *ref, int *x, int *y, int z);
void far Npc_setTargetWeapon(objref *ref, unsigned value);
void far Npc_setMale(objref *ref);
void far Npc_setFemale(objref *ref);
unsigned char far Npc_isMale(objref *ref);
unsigned char far IsNpcUnconscious(objref *ref);

unsigned char FindAvatarNpc(void);
void Npc_clearTargets(objref *who);

void Npc_runSchedule(objref *who);

/* The NPC's last path failed, or type flag 8 is set. */
inline char IsBlocked(objref *who)
{
	return GetNpcBufferForIbo(who)->result == -1 || GetNpcBufferForIbo(who)->marked(8);
}

/* Accessors for an NPC's buffer record. */
inline unsigned char IsInParty(objref *who) { return (GetNpcBufferForIbo(who)->status & NPC_IN_PARTY) != 0; }
inline unsigned char IsDead(objref *who) { return (GetNpcBufferForIbo(who)->status & NPC_DEAD) != 0; }
inline unsigned char GetAlignment(objref *who) { return (GetNpcBufferForIbo(who)->status & 0x18) >> 3; }
inline unsigned char GetFacing(objref *who) { return GetNpcBufferForIbo(who)->status & 7; }
inline unsigned char HasTarget(objref *who) { return GetNpcBufferForIbo(who)->primaryTarget != -1; }
inline unsigned char HasSecondary(objref *who) { return GetNpcBufferForIbo(who)->secondaryTarget != -1; }
inline unsigned char WantsPrimary(objref *who) { return GetNpcBufferForIbo(who)->typeFlagsHigh & 1; }
inline unsigned char HasStatus(objref *who, unsigned mask) { return (GetNpcBufferForIbo(who)->status & mask) != 0; }
inline void ChangeStatus(objref *who, unsigned clear, unsigned set)
{
	NpcBuffer far *npc = GetNpcBufferForIbo(who);
	npc->status &= ~clear;
	npc->status |= set;
}
inline unsigned char IsOnOutermostSchedule(objref *who) { return GetNpcBufferForIbo(who)->currentSchedule == 0; }
inline unsigned char IsInCombat(objref *who)
{
	return GetNpcBufferForIbo(who)->workType == WORK_COMBAT && IsOnOutermostSchedule(who);
}

/* Food 10 and up is fed, 5 to 9 hungry, 1 to 4 starving and 0 famished. */
inline unsigned char IsWellFed(objref *who) { return GetNpcBufferForIbo(who)->food >= 10; }
inline unsigned char IsHungry(objref *who)
{
	return GetNpcBufferForIbo(who)->food < 10 && GetNpcBufferForIbo(who)->food >= 5;
}
inline unsigned char IsStarving(objref *who)
{
	return GetNpcBufferForIbo(who)->food < 5 && GetNpcBufferForIbo(who)->food > 0;
}
inline unsigned char IsFamished(objref *who) { return GetNpcBufferForIbo(who)->food == 0; }

void far DetachItem(int item);
void far SendNPCToLunch(objref npc);

extern char *U7NBufFileName;
extern NpcSaver NpcBufferFile;

#endif
