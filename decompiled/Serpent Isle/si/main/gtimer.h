#ifndef GTIMER_H
#define GTIMER_H

#include "datanode.h"

#define TICKS_PER_MINUTE    25
#define TICKS_PER_HOUR      1500
#define TICKS_PER_DAY       36000U

/* A calendar date; it can be kept in a file of its own. */
struct GameDate {
	unsigned year;
	unsigned month;
	unsigned day;
	GameDate();
	int load(char *name);
	int save(char *name);
};

/* The game clock: 25 ticks a minute, 1500 an hour; 7-day weeks, 28-day months, 13 months a year. */
class GameTimer : public DataNode {
public:
	char filename[14];
	unsigned ticks;     /* since midnight */
	unsigned days;
	unsigned rate;      /* ticks per step */
	int hold;           /* steps left before the clock runs again */

	unsigned char running() { return hold == 0; }
	GameTimer();
	GameTimer(int hour, int step) { init(hour, step); }
	void init(int hour, int step);
	unsigned long getElapsed();
	unsigned getTicks();
	unsigned getMinute();
	unsigned getHour();
	unsigned getDay();
	unsigned getMonth();
	unsigned getYear();
	unsigned getTotalDays();
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

int IsNight(void);

extern char *GameTimerFileName;
extern GameTimer GameTime;
extern char TimerFileLetter;

inline unsigned char IsDaytime() { return GameTime.getHour() > 4 && GameTime.getHour() < 21; }

#endif
