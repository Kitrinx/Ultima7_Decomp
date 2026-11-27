/* Serpent Isle SI.EXE, resident segment 21 (file offsets 0x01383c to 0x013abf, 643 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include "plat.h"
#include "iteminfo.h"
#include "objref.h"
#include "u7event.h"
#include "cheat.h"
#include "attack.h"
#include "preload.h"
#include "voice.h"
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
#include "u7sound.h"
#include "rules.h"
#include "worldpal.h"

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
		if (SpeechPlaysDuringGame())
			SpeechPlayer.continuePlaying();
	}
	while (SpeechPlaysDuringGame() && SpeechPlayer.isPlaying()) {
		plat_yield();
		SpeechPlayer.continuePlaying();
	}
	TimeAdvanceRate = 0;
}

static uint32_t PreviousFrameTime = 0;
static uint16_t FrameTickRemainder = 0;

void WaitForFrameTime(void)
{
	uint16_t fps = (uint16_t) plat_setting("frame_limit");
	uint16_t ticks = PLAT_TICKS_PER_SECOND + FrameTickRemainder;
	uint16_t delay = ticks / fps;

	/* Carry fractional ticks so rates like 7 FPS average out on the 60 Hz timer. */
	FrameTickRemainder = ticks % fps;
	while ((uint16_t)(TickCount - PreviousFrameTime) < delay) {
		GameScreen.stepLighting();
		plat_yield();
	}
	PreviousFrameTime = TickCount;
}

void RunGameStep(void)
{
	FlushKeyboard();
	SetPointerZ(Item_getZ(&AvatarRef));
	if (ShowAvatarLocation)
		ConsolePrintAt(1, 1, "L=(%x,%x,%x)", (int16_t) Item_getX(AvatarRef),
			(int16_t) Item_getY(AvatarRef), Item_getZ(&AvatarRef));
	if (SpeechPlaysDuringGame())
		SpeechPlayer.continuePlaying();
	AutorouteActive = ContinueAutoroute();
	UpdatePartyFollow();
	if ((int8_t) (GameTime.ticks % TICKS_PER_HOUR == 0) || !PendingSteps)
		UpdateNPCStatus();
	RunActionQueue();
	UpdateNPCs(PendingSteps > 0);
	if (!PendingSteps && TimeAdvanceRate == 0) {
		WaitForFrame();
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
	if (MusicChangeDue)
		PlayMusic(RequestedMusic);
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

extern "C" void ResetMainGlobals(void)
{
	RestartRequested = 0;
	EndgameRequested = 0;
	PendingSteps = 0;
	EndgameQuitRequested = 0;
	TimeAdvanceRate = 0;
	PreviousFrameTime = 0;
	FrameTickRemainder = 0;
}
