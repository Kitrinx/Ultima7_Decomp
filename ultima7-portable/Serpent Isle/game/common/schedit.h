#ifndef SCHEDIT_H
#define SCHEDIT_H

struct Schedules;

uint8_t Schedule_write(Schedules *s, char *name);
uint8_t Schedule_clearEntry(Schedules *s, uint8_t npc, uint8_t n);
uint8_t Schedule_insertEntry(Schedules *s, uint8_t npc);
uint8_t Schedule_removeEntry(Schedules *s, uint8_t npc, uint8_t n);
void Schedule_setEntryTime(Schedules *s, uint8_t npc, uint8_t n, uint8_t time);
void Schedule_setEntryWorkType(Schedules *s, uint8_t npc, uint8_t n, uint8_t type);
void Schedule_setEntryPlace(Schedules *s, uint8_t npc, uint8_t n, uint8_t x,
	uint8_t y, uint8_t region);

/* chngsche.c: the saved game's changed schedules */
void RevertSchedule(int16_t npc);

#endif
