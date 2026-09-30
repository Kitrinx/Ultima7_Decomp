#ifndef SCHE_H
#define SCHE_H

struct Coord;

/* one entry: from time on, the NPC does type at x, y of region */
struct ScheduleEntry {
	uint8_t time:3;
	uint8_t type:5;
	uint8_t x;
	uint8_t y;
	uint8_t region;
};

/* every NPC's schedules: NPC i owns entries index[i] up to index[i + 1] */
struct Schedules {
	uint16_t count;
	uint16_t *index;
	ScheduleEntry *entries;
	Schedules() { index = 0; entries = 0; }
};

extern char *ScheduleFileName;
extern char *ScheduleTempFileName;
extern Schedules ScheduleTable;
uint8_t Schedule_hasEntry(Schedules *s, uint8_t npc, uint8_t n);

uint8_t Schedule_getWorkType(Schedules *s, uint8_t npc, uint8_t time);
uint8_t Schedule_getPlace(Schedules *s, uint8_t npc, uint8_t time, uint8_t *x,
	uint8_t *y, uint8_t *region);
uint8_t Schedule_getCoord(Schedules *s, uint8_t npc, uint8_t time, Coord *x, Coord *y);
uint8_t Schedule_findEntry(Schedules *s, uint8_t npc, uint8_t time);
uint8_t Schedule_getEntryWorkType(Schedules *s, uint8_t npc, uint8_t time);
int16_t Schedule_getEntryX(Schedules *s, uint8_t npc, uint8_t time);
int16_t Schedule_getEntryY(Schedules *s, uint8_t npc, uint8_t time);
int16_t Schedule_getEntryRegion(Schedules *s, uint8_t npc, uint8_t time);

/* What an NPC does in each kind of schedule, and the hourly schedule update. */
struct NPCRef;
struct objref;

void RunLabSchedule(objref *npc);
void DoWorkArchery(objref *npc);
void DoWorkCheckArea(objref *npc);
extern uint8_t BakedFoodFrames[6];
void RunBakeSchedule(objref *npc);
void RunDanceSchedule(objref *npc);
void RunDeskWorkSchedule(objref *npc);
void RunEatAtInnSchedule(objref *npc);
void RunFarmSchedule(objref *npc);
void DoWorkFencing(objref *npc);
void DoWorkFillBucket(objref *npc);
void RunShySchedule(objref *npc);
void RunEatSchedule(objref *npc);
void RunBlacksmithSchedule(objref *npc);
void RunGrazeSchedule(objref *npc);
void DoWorkReadyHand(objref *npc);
void DoWorkHound(objref *npc);
void RunKidGamesSchedule(objref *npc);
void DoWorkCheckLight(objref *npc);
void RunLoiterSchedule(NPCRef *npc);
void RunMinerSchedule(objref *npc);
void RunPaceSchedule(objref *npc);
extern char GraveFlowerFrames[5];
void ResumeWaitingPatrols();
void RunPatrolSchedule(objref *npc);
void RunDuelSchedule(objref *npc);
void RunPreachSchedule(objref *npc);
void DoWorkRead(objref *npc);
void DoWorkPayRespects(objref *npc);
void RunSewSchedule(objref *npc);
void RunTendShopSchedule(NPCRef *npc);
void DoWorkCheckShutters(objref *npc);
extern int16_t NapBed;
extern int16_t PartySitRefs[9];
void RunMajorSitSchedule(objref *npc);
void DoWorkSit(objref *npc);
void RunSleepSchedule(objref *npc);
void RunStandSchedule(objref *npc);
void DoWorkTag(objref *npc);
void RunTalkSchedule(objref *npc);
void RunThiefSchedule(objref *npc);
void RunWanderSchedule(NPCRef *npc);
void DoWorkFillWaterTrough(objref *npc);
extern int16_t unused_global_1;
void ResetNpcSchedules(void);
void UpdateNpcSchedules(void);

#ifdef __cplusplus
extern "C" {
#endif
void RunSpecialSchedule(void);
void RunWaitSchedule(void);
#ifdef __cplusplus
}
#endif

#endif
