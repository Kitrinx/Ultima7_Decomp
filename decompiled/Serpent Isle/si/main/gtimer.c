/* Serpent Isle SI.EXE, resident segment 15 (file offsets 0x011095 to 0x01138a, 757 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include <string.h>
#include "dosio.h"
#include "easyfile.h"
#include "datanode.h"
#include "gtimer.h"

char *GameTimerFileName = "GAMETIM_.DAT";
GameTimer GameTime(6, 1);
char TimerFileLetter = 'A';     /* the letter that tells clocks' files apart */

void GameTimer::init(int hour, int step)
{
	ticks = hour * TICKS_PER_HOUR;
	days = 0;
	rate = step;
	strncpy(filename, GameTimerFileName, sizeof filename);
	filename[7] = TimerFileLetter++;
}

unsigned long GameTimer::getElapsed()
{
	return (days * TICKS_PER_DAY + ticks) / rate;
}

unsigned GameTimer::getTicks()
{
	return ticks;
}

unsigned GameTimer::getMinute()
{
	return ticks / TICKS_PER_MINUTE % 60;
}

unsigned GameTimer::getHour()
{
	return ticks / TICKS_PER_HOUR % 24;
}

unsigned GameTimer::getDay()
{
	return days % 7;
}

unsigned GameTimer::getMonth()
{
	return days / 28 % 13;
}

unsigned GameTimer::getYear()
{
	return days / 364;
}

unsigned GameTimer::getTotalDays()
{
	return days;
}

void GameTimer::getDate(GameDate *d)
{
	d->year = days / 364;
	d->month = days % 364 / 28;
	d->day = days % 364 % 28;
}

char *GameTimer::name()
{
	return filename;
}

void GameTimer::save(char *dir)
{
	int h;

	h = CreateFileOrFail(BuildPath(dir, filename, 0));
	DosWrite(h, -1L, 2L, &ticks);
	DosWrite(h, -1L, 2L, &days);
	DosWrite(h, -1L, 2L, &rate);
	DosClose(h);
}

void GameTimer::refresh(char *dir)
{
	save(dir);
}

void GameTimer::load(char *dir)
{
	int h;

	h = DosOpen(BuildPath(dir, filename, 0));
	if (h != -1) {
		DosRead(h, -1L, 2L, &ticks);
		DosRead(h, -1L, 2L, &days);
		DosRead(h, -1L, 2L, &rate);
	}
	DosClose(h);
}

void GameTimer::tick()
{
	if (!running())
		hold--;
	else {
		ticks += rate;
		while (ticks >= TICKS_PER_DAY) {
			ticks -= TICKS_PER_DAY;
			days++;
		}
	}
}

/* true from 9 in the evening until 5 in the morning */
int IsNight(void)
{
	return GameTime.getHour() < 5 || GameTime.getHour() > 20;
}
