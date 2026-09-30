/* Steps the red screen's palette from the game timer. */

#include "u7port.h"
#include "plat.h"
#include "redscrn.h"
#include "redtimer.h"

int32_t RedScreenStepTicks = 1;         /* ticks between palette steps */
static int32_t Countdown = 0;           /* ticks until the next palette step */
static int16_t Cycling = 0;

static void RedScreenTick(void)
{
	if (Cycling && --Countdown == 0) {
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
}
