/* Steps the red screen's palette from the game timer. */

#include "u7port.h"
#include "plat.h"
#include "redscrn.h"
#include "redtimer.h"

int32_t RedScreenStepTicks = 1;         /* ticks between palette steps */
static int32_t Countdown = 0;           /* ticks until the next palette step */
static int16_t Cycling = 0;
static uint32_t LastBiosTick = 0;

/* The hook ran on the 18.2 Hz BIOS tick and stepped on every other one. */
static void RedScreenTick(void)
{
	uint32_t bios = (uint32_t)((uint64_t)plat_milliseconds() * 1193182 / 65536000);

	if (!Cycling || bios - LastBiosTick < 2)
		return;
	LastBiosTick = bios;
	if (--Countdown == 0) {
		Countdown = RedScreenStepTicks;
		CycleRedScreenPalette();
	}
}

/* The timer runs only while cycling, so nothing reaches the host before the game starts. */
extern "C" void HookRedScreenTimer(void)
{
	Countdown = RedScreenStepTicks;
	Cycling = 0;
}

extern "C" void UnhookRedScreenTimer(void)
{
	StopRedScreenCycle();
}

extern "C" void StartRedScreenCycle(void)
{
	if (!Cycling)
		plat_game_timer_add(RedScreenTick);
	Cycling = 1;
}

extern "C" void StopRedScreenCycle(void)
{
	if (Cycling)
		plat_game_timer_remove(RedScreenTick);
	Cycling = 0;
}

extern "C" void ResetRedtimerGlobals(void)
{
	RedScreenStepTicks = 1;
	Countdown = 0;
	Cycling = 0;
	LastBiosTick = 0;
}
