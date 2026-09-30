/* Black Gate U7.EXE, resident segment 30 (file offsets 0x018de0 to 0x01907e, 670 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Original folder unknown.
 */

#include "u7port.h"
#include "plat.h"
#include "systimer.h"
#include "iteminfo.h"
#include "objref.h"
#include "u7event.h"
#include "cheat.h"
#include "voice.h"
#include "attack.h"
#include "preload.h"
#include "gtimer.h"
#include "u7point.h"
#include "savegame.h"
#include "actqueue.h"
#include "sche.h"
#include "itable.h"
#include "movepath.h"
#include "partymov.h"
#include "speed.h"
#include "camera.h"
#include "mainctrl.h"
#include "colbuf.h"
#include "init.h"
#include "item.h"

extern objref AvatarRef;
extern uint8_t ShouldExitMainGameLoop;

uint8_t RestartRequested = 0;
uint8_t EndgameRequested = 0;
uint8_t PendingSteps = 0;
uint8_t EndgameQuitRequested = 0;
int16_t TimeAdvanceRate = 0;

void AdvanceTime(void)
{
	int16_t savedRate;
	int16_t i;

	savedRate = GameTime.rate;
	GameTime.rate = TimeAdvanceRate;
	GameTime.tick();
	GameTime.rate = savedRate;
	UpdateNpcSchedules();
	for (i = 0; i < 30; i++) {
		UpdateNPCs(0);
		GameTime.tick();
		RunActionQueue();
		SpeechPlayer.continuePlaying();
	}
	while (SpeechPlayer.isPlaying()) {
		plat_yield();
		SpeechPlayer.continuePlaying();
	}
	TimeAdvanceRate = 0;
}

/* World draws, and so game steps, are held to one per FrameDelay ticks of the 60 Hz timer:
 * 10 a second. The shipped game ran as fast as the machine could draw. */
uint16_t FrameDelay = 6;
static uint32_t PreviousFrameTime = 0;

void WaitForFrameTime(void)
{
	while ((uint16_t)(TickCount - PreviousFrameTime) < FrameDelay)
		plat_yield();
	PreviousFrameTime = TickCount;
}

void RunGameStep(void)
{
	FlushKeyboard();
	SetPointerZ(Item_getZ(&AvatarRef));
	if (ShowAvatarLocation)
		ConsolePrintAt(1, 1, "L=(%x,%x,%x)", (int16_t) Item_getX(AvatarRef),
			(int16_t) Item_getY(AvatarRef), Item_getZ(&AvatarRef));
	SpeechPlayer.continuePlaying();
	AutorouteActive = ContinueAutoroute();
	UpdatePartyFollow();
	if ((int8_t) (GameTime.ticks % TICKS_PER_HOUR == 0) || !PendingSteps)
		UpdateNPCStatus();
	RunActionQueue();
	UpdateNPCs(PendingSteps > 0);
	if (!PendingSteps && TimeAdvanceRate == 0) {
		if (FrameRateShown)
			MeasureFrameRate();
		WaitForFrameTime();
		DrawWorld(&gCamera);
	}
	GameTime.tick();
	if (TimeAdvanceRate == 0)
		PollAndProcessKey(&PendingSteps);
	else
		AdvanceTime();
}

extern "C" void MainGameLoop(void)
{
	while (!ShouldExitMainGameLoop) {
		AdjustCursorZ(Item_getZ(&AvatarRef) - CursorCenterZ);
		gCamera.setTarget(AvatarRef);
		DrawWorld(&gCamera);
		/* clear the avatar's scripted flag */
		Item_setQualityFlags(&AvatarRef, Item_getQualityFlags(&AvatarRef) & 0xdf);
		RestartRequested = 0;
		EndgameRequested = 0;
		while (!ShouldExitMainGameLoop && !RestartRequested && !EndgameRequested && !EndgameQuitRequested)
			RunGameStep();
		if (RestartRequested) {
			SaveGameArgs();
			SaveGameFiles.deleteGameDirectory();
			QuitToDos();
		}
		if (EndgameRequested)
			EndGame();
		if (EndgameQuitRequested)
			ExitForEndgame();
	}
}
