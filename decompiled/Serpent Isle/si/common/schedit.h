#ifndef SCHEDIT_H
#define SCHEDIT_H

struct Schedules;

unsigned char Schedule_write(Schedules *s, char *name);
unsigned char Schedule_clearEntry(Schedules *s, unsigned char npc, unsigned char n);
unsigned char Schedule_insertEntry(Schedules *s, unsigned char npc);
unsigned char Schedule_removeEntry(Schedules *s, unsigned char npc, unsigned char n);
void Schedule_setEntryTime(Schedules *s, unsigned char npc, unsigned char n, unsigned char time);
void Schedule_setEntryWorkType(Schedules *s, unsigned char npc, unsigned char n, unsigned char type);
void Schedule_setEntryPlace(Schedules *s, unsigned char npc, unsigned char n, unsigned char x,
	unsigned char y, unsigned char region);

/* chngsche.c: the saved game's changed schedules */
void RevertSchedule(int npc);

#endif
