#ifndef SCHE_OV1_H
#define SCHE_OV1_H

struct Schedules;

extern Schedules ScheduleTable;

void InitSchedules(void);

uint8_t Schedule_resize(Schedules *s, uint16_t n);
uint8_t Schedule_read(Schedules *s, char *name);
uint8_t Schedule_write(Schedules *s, char *name);
void Schedule_init(Schedules *s, uint16_t n);

#endif
