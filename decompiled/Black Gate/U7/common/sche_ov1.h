#ifndef SCHE_OV1_H
#define SCHE_OV1_H

struct Schedules;

extern Schedules ScheduleTable;

void InitSchedules(void);

unsigned char Schedule_resize(Schedules *s, unsigned n);
unsigned char Schedule_read(Schedules *s, char *name);
unsigned char Schedule_write(Schedules *s, char *name);
void Schedule_init(Schedules *s, unsigned n);

#endif
