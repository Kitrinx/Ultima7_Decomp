#ifndef SCHE_H
#define SCHE_H

struct Coord;

/* one entry: from time on, the NPC does type at x, y of region */
struct ScheduleEntry {
	unsigned char time:3;
	unsigned char type:5;
	unsigned char x;
	unsigned char y;
	unsigned char region;
};

/* every NPC's schedules: NPC i owns entries index[i] up to index[i + 1] */
struct Schedules {
	unsigned count;
	unsigned far *index;
	ScheduleEntry far *entries;
	Schedules() { index = 0; entries = 0; }
};

extern char *ScheduleFileName;
extern char *ScheduleTempFileName;
extern Schedules ScheduleTable;
unsigned char Schedule_hasEntry(Schedules *s, unsigned char npc, unsigned char n);

unsigned char Schedule_getWorkType(Schedules *s, unsigned char npc, unsigned char time);
unsigned char Schedule_getPlace(Schedules *s, unsigned char npc, unsigned char time, unsigned char *x,
	unsigned char *y, unsigned char *region);
unsigned char Schedule_getCoord(Schedules *s, unsigned char npc, unsigned char time, Coord *x, Coord *y);
unsigned char Schedule_findEntry(Schedules *s, unsigned char npc, unsigned char time);
unsigned char Schedule_getEntryWorkType(Schedules *s, unsigned char npc, unsigned char time);
int Schedule_getEntryX(Schedules *s, unsigned char npc, unsigned char time);
int Schedule_getEntryY(Schedules *s, unsigned char npc, unsigned char time);
int Schedule_getEntryRegion(Schedules *s, unsigned char npc, unsigned char time);

/* What an NPC does in each kind of schedule, and the hourly schedule update. */
struct NPCRef;
struct objref;

void far RunLabSchedule(objref *npc);
void far DoWorkArchery(objref *npc);
void far DoWorkCheckArea(objref *npc);
extern unsigned char BakedFoodFrames[6];
void far RunBakeSchedule(objref *npc);
void far RunDanceSchedule(objref *npc);
void far RunDeskWorkSchedule(objref *npc);
void far RunEatAtInnSchedule(objref *npc);
void far RunFarmSchedule(objref *npc);
void far DoWorkFencing(objref *npc);
void far DoWorkFillBucket(objref *npc);
void far RunShySchedule(objref *npc);
void far RunEatSchedule(objref *npc);
void far RunBlacksmithSchedule(objref *npc);
void far RunGrazeSchedule(objref *npc);
void far DoWorkReadyHand(objref *npc);
void far DoWorkHound(objref *npc);
void far RunKidGamesSchedule(objref *npc);
void far DoWorkCheckLight(objref *npc);
void far RunLoiterSchedule(NPCRef *npc);
void far RunMinerSchedule(objref *npc);
void far RunPaceSchedule(objref *npc);
extern char GraveFlowerFrames[5];
void far ResumeWaitingPatrols();
void far RunPatrolSchedule(objref *npc);
void far RunDuelSchedule(objref *npc);
void far RunPreachSchedule(objref *npc);
void far DoWorkRead(objref *npc);
void far DoWorkPayRespects(objref *npc);
void far RunSewSchedule(objref *npc);
void far RunTendShopSchedule(NPCRef *npc);
void far DoWorkCheckShutters(objref *npc);
extern int NapBed;
extern int PartySitRefs[8];
void far RunMajorSitSchedule(objref *npc);
void far DoWorkSit(objref *npc);
void far RunSleepSchedule(objref *npc);
void far RunStandSchedule(objref *npc);
void far DoWorkTag(objref *npc);
void far RunTalkSchedule(objref *npc);
void far RunThiefSchedule(objref *npc);
void far RunWanderSchedule(NPCRef *npc);
void far DoWorkFillWaterTrough(objref *npc);
extern int unused_global_1;
void far ResetNpcSchedules(void);
void far UpdateNpcSchedules(void);

void far RunPathfindSchedule(objref *npc);

#ifdef __cplusplus
extern "C" {
#endif
void RunWaitSchedule(void);
#ifdef __cplusplus
}
#endif

#endif
