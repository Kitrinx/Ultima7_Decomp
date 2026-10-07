/* Serpent Isle SI.EXE, resident segment 15 (file offsets 0x011095 to 0x01138a, 757 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include <new>
#include <string.h>
#include "dosio.h"
#include "easyfile.h"
#include "datanode.h"
#include "gtimer.h"

char *const GameTimerFileName = "GAMETIM_.DAT";
GameTimer GameTime(6, 1);
int8_t TimerFileLetter = 'A';     /* the letter that tells clocks' files apart */

void GameTimer::init(int16_t hour, int16_t step)
{
	ticks = hour * TICKS_PER_HOUR;
	days = 0;
	rate = step;
	strncpy(filename, GameTimerFileName, sizeof filename);
	filename[7] = TimerFileLetter++;
}

uint32_t GameTimer::getElapsed()
{
	return (days * TICKS_PER_DAY + ticks) / rate;
}

uint16_t GameTimer::getTicks()
{
	return ticks;
}

uint16_t GameTimer::getMinute()
{
	return ticks / TICKS_PER_MINUTE % 60;
}

uint16_t GameTimer::getHour()
{
	return ticks / TICKS_PER_HOUR % 24;
}

uint16_t GameTimer::getDay()
{
	return days % 7;
}

uint16_t GameTimer::getMonth()
{
	return days / 28 % 13;
}

uint16_t GameTimer::getYear()
{
	return days / 364;
}

uint16_t GameTimer::getTotalDays()
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
	int16_t h;

	h = CreateFileOrFail(BuildPath(dir, filename, 0));
	DosWrite(h, -INT32_C(1), INT32_C(2), &ticks);
	DosWrite(h, -INT32_C(1), INT32_C(2), &days);
	DosWrite(h, -INT32_C(1), INT32_C(2), &rate);
	DosClose(h);
}

void GameTimer::refresh(char *dir)
{
	save(dir);
}

void GameTimer::load(char *dir)
{
	int16_t h;

	h = DosOpen(BuildPath(dir, filename, 0));
	if (h != -1) {
		DosRead(h, -INT32_C(1), INT32_C(2), &ticks);
		DosRead(h, -INT32_C(1), INT32_C(2), &days);
		DosRead(h, -INT32_C(1), INT32_C(2), &rate);
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
int16_t IsNight(void)
{
	return GameTime.getHour() < 5 || GameTime.getHour() > 20;
}

extern "C" void ResetGtimerGlobals(void)
{
	memset((void *)&GameTime, 0, sizeof(GameTime));
	TimerFileLetter = 'A';
}

extern "C" void ConstructGtimerGlobals(void)
{
	new (&GameTime) GameTimer(6, 1);
}
