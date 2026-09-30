#ifndef GTIMER_H
#define GTIMER_H

#include "datanode.h"

#define TICKS_PER_MINUTE    25
#define TICKS_PER_HOUR      1500
#define TICKS_PER_DAY       36000U

/* A calendar date; it can be kept in a file of its own. */
struct GameDate {
	uint16_t year;
	uint16_t month;
	uint16_t day;
	GameDate();
	int16_t load(char *name);
	int16_t save(char *name);
};

/* The game clock: 25 ticks a minute, 1500 an hour; 7-day weeks, 28-day months, 13 months a year. */
class GameTimer : public DataNode {
public:
	char filename[14];
	uint16_t ticks;     /* since midnight */
	uint16_t days;
	uint16_t rate;      /* ticks per step */
	int16_t hold;           /* steps left before the clock runs again */

	uint8_t running() { return hold == 0; }
	GameTimer();
	GameTimer(int16_t hour, int16_t step) { init(hour, step); }
	void init(int16_t hour, int16_t step);
	uint32_t getElapsed();
	uint16_t getTicks();
	uint16_t getMinute();
	uint16_t getHour();
	uint16_t getDay();
	uint16_t getMonth();
	uint16_t getYear();
	void getDate(GameDate *d);
	char *name();
	void save(char *dir);
	void refresh(char *dir);
	void load(char *dir);
	void tick();

	/* Copies the time but keeps the file name and save-list link. */
	void operator=(GameTimer source)
	{
		ticks = source.ticks;
		days = source.days;
		rate = source.rate;
		hold = source.hold;
	}
};

extern char *GameTimerFileName;
extern GameTimer GameTime;
extern int8_t TimerFileLetter;

inline int8_t IsDaytime() { return GameTime.getHour() > 4 && GameTime.getHour() < 21; }

#endif
